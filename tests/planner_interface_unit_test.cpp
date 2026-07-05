#include <humanoid/planner/IPlanner.h>
#include <humanoid/planner/PlannerFactory.h>
#include <humanoid/planner/PlannerRegistry.h>

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <latch>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

void Require(bool condition) {
  if (!condition) {
    std::abort();
  }
}

[[nodiscard]] humanoid::planner::PlannerCapabilities MakeCapabilities(std::string planner_id) {
  humanoid::planner::PlannerCapabilities capabilities;
  capabilities.plannerId = std::move(planner_id);
  capabilities.name = "Test Planner";
  capabilities.description = "Planner interface unit test";
  capabilities.kind = humanoid::planner::PlannerKind::Rule;
  capabilities.supportedGoalTypes = {humanoid::planner::GoalType::Mission,
                                     humanoid::planner::GoalType::BehaviorTree};
  capabilities.supportsMissionOutput = true;
  capabilities.supportsBehaviorTreeOutput = true;
  capabilities.supportsPlanValidation = true;
  capabilities.supportsCancellation = true;
  return capabilities;
}

class TestPlanner final : public humanoid::planner::IPlanner {
public:
  explicit TestPlanner(humanoid::planner::PlannerCapabilities capabilities)
      : capabilities_(std::move(capabilities)) {}

  [[nodiscard]] humanoid::planner::PlanningResult
  Plan(const humanoid::planner::PlanningRequest& request) override {
    humanoid::planner::PlanningResult result;
    if (!request.isValid()) {
      result.status = humanoid::planner::GoalStatus::Rejected;
      return result;
    }

    humanoid::mission::Mission mission;
    mission.id = request.goal.id;
    mission.name = request.goal.description;
    result.mission = std::move(mission);
    result.status = humanoid::planner::GoalStatus::Planned;
    return result;
  }

  [[nodiscard]] humanoid::common::Status
  ValidatePlan(const humanoid::planner::PlanningResult& result) const override {
    if (!result.isSuccess()) {
      return humanoid::common::Status::error(humanoid::common::StatusCode::kInvalidArgument,
                                             "planning result is not successful");
    }

    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status CancelPlan() override {
    cancelled_ = true;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::planner::PlannerCapabilities GetCapabilities() const override {
    return capabilities_;
  }

  [[nodiscard]] bool Cancelled() const noexcept { return cancelled_; }

private:
  humanoid::planner::PlannerCapabilities capabilities_;
  bool cancelled_{false};
};

[[nodiscard]] humanoid::planner::PlannerFactory::PlannerCreator
MakeCreator(humanoid::planner::PlannerCapabilities capabilities) {
  return [capabilities = std::move(capabilities)]() {
    return std::make_unique<TestPlanner>(capabilities);
  };
}

void VerifyRegistry() {
  humanoid::planner::PlannerRegistry registry;
  humanoid::planner::PlannerCapabilities capabilities =
      MakeCapabilities("org.humanoid.planner.registry");

  Require(!registry.RegisterPlanner(humanoid::planner::PlannerCapabilities{}).isOk());
  Require(registry.RegisterPlanner(capabilities).isOk());
  Require(!registry.RegisterPlanner(capabilities).isOk());
  Require(registry.Contains(capabilities.plannerId));
  Require(registry.PlannerCount() == 1U);

  const std::optional<humanoid::planner::PlannerCapabilities> stored =
      registry.Capabilities(capabilities.plannerId);
  Require(stored.has_value());
  Require(stored->plannerId == capabilities.plannerId);
  Require(stored->supportsMissionOutput);

  const std::vector<humanoid::planner::PlannerRecord> records = registry.EnumeratePlanners();
  Require(records.size() == 1U);
  Require(records.front().capabilities.plannerId == capabilities.plannerId);

  Require(registry.UnregisterPlanner(capabilities.plannerId).isOk());
  Require(!registry.Contains(capabilities.plannerId));
}

void VerifyFactoryDependencyInjectionAndLifecycle() {
  auto registry = std::make_shared<humanoid::planner::PlannerRegistry>();
  humanoid::planner::PlannerFactory factory{registry};
  humanoid::planner::PlannerCapabilities capabilities =
      MakeCapabilities("org.humanoid.planner.factory");

  Require(factory.RegisterPlanner(capabilities, {}).code() ==
          humanoid::common::StatusCode::kInvalidArgument);
  Require(factory.RegisterPlanner(capabilities, MakeCreator(capabilities)).isOk());
  Require(factory.RegisteredPlannerCount() == 1U);
  Require(registry->Contains(capabilities.plannerId));

  humanoid::planner::PlannerCreationResult created = factory.CreatePlanner(capabilities.plannerId);
  Require(created.status.isOk());
  Require(static_cast<bool>(created.planner));

  Require(!factory.UnregisterPlanner(capabilities.plannerId).isOk());

  humanoid::planner::PlanningRequest request;
  request.goal.id = 11U;
  request.goal.description = "Create an inspection mission";
  request.goal.type = humanoid::planner::GoalType::Mission;
  humanoid::planner::PlanningResult result = created.planner->Plan(request);
  Require(result.isSuccess());
  Require(created.planner->ValidatePlan(result).isOk());
  Require(created.planner->CancelPlan().isOk());

  Require(factory.DestroyPlanner(created.planner).isOk());
  Require(!created.planner);
  Require(factory.UnregisterPlanner(capabilities.plannerId).isOk());
}

void VerifyFactoryRejectsInvalidCreators() {
  humanoid::planner::PlannerFactory factory;
  humanoid::planner::PlannerCapabilities capabilities =
      MakeCapabilities("org.humanoid.planner.invalid");

  Require(factory.CreatePlanner("missing").status.code() ==
          humanoid::common::StatusCode::kFailedPrecondition);

  Require(factory
              .RegisterPlanner(capabilities,
                               []() { return std::unique_ptr<humanoid::planner::IPlanner>{}; })
              .isOk());

  Require(factory.CreatePlanner(capabilities.plannerId).status.code() ==
          humanoid::common::StatusCode::kInternalError);

  humanoid::planner::PlannerCapabilities expected =
      MakeCapabilities("org.humanoid.planner.expected");
  humanoid::planner::PlannerCapabilities actual = MakeCapabilities("org.humanoid.planner.actual");
  Require(factory.RegisterPlanner(expected, MakeCreator(actual)).isOk());
  Require(factory.CreatePlanner(expected.plannerId).status.code() ==
          humanoid::common::StatusCode::kInternalError);
}

void VerifyConcurrentRegistryAccess() {
  constexpr std::size_t kPlannerCount{16};

  humanoid::planner::PlannerRegistry registry;
  std::latch start_gate{1};
  std::atomic<std::size_t> success_count{0};
  std::vector<std::jthread> workers;
  workers.reserve(kPlannerCount);

  for (std::size_t index = 0; index < kPlannerCount; ++index) {
    workers.emplace_back([&registry, &start_gate, &success_count, index]() {
      start_gate.wait();
      humanoid::planner::PlannerCapabilities capabilities =
          MakeCapabilities("org.humanoid.planner.concurrent." + std::to_string(index));
      if (registry.RegisterPlanner(std::move(capabilities)).isOk()) {
        success_count.fetch_add(1U, std::memory_order_relaxed);
      }
    });
  }

  start_gate.count_down();
  workers.clear();

  Require(success_count.load(std::memory_order_relaxed) == kPlannerCount);
  Require(registry.PlannerCount() == kPlannerCount);
}

} // namespace

int main() {
  VerifyRegistry();
  VerifyFactoryDependencyInjectionAndLifecycle();
  VerifyFactoryRejectsInvalidCreators();
  VerifyConcurrentRegistryAccess();
  return 0;
}
