#include <humanoid/adapters/IRobotAdapter.h>
#include <humanoid/bt/BTContext.h>
#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BehaviorTree.h>
#include <humanoid/cloud/CloudPlatform.h>
#include <humanoid/common/Status.hpp>
#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/core/CommandExecutionPipeline.h>
#include <humanoid/core/CommandResult.h>
#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/mission/MissionExecutor.h>
#include <humanoid/mission/MissionStep.h>
#include <humanoid/perception/PerceptionPipeline.h>
#include <humanoid/planner/RuleBasedPlanner.h>
#include <humanoid/plugins/PluginRegistry.hpp>
#include <humanoid/runtime/RuntimeScheduler.h>
#include <humanoid/services/TelemetryService.h>

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct BenchmarkResult final {
  std::string name;
  std::size_t iterations{0U};
  double totalMilliseconds{0.0};
  double averageMicroseconds{0.0};
};

template <typename Callable>
BenchmarkResult Measure(std::string name, std::size_t iterations, Callable callable) {
  const auto start = Clock::now();
  for (std::size_t index = 0; index < iterations; ++index) {
    callable(index);
  }
  const auto elapsed = Clock::now() - start;
  const auto total_microseconds =
      std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(elapsed).count();
  return BenchmarkResult{std::move(name), iterations, total_microseconds / 1000.0,
                         total_microseconds / static_cast<double>(iterations)};
}

humanoid::core::Command MakeCommand(humanoid::core::CommandId id,
                                    humanoid::core::CommandType type) {
  humanoid::core::Command command{};
  command.id = id;
  command.timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(Clock::now());
  command.type = type;
  return command;
}

humanoid::core::CommandResult MakeCompletedCommandResult(std::string message = "ok") {
  humanoid::core::CommandResult result{};
  result.status = humanoid::core::CommandStatus::Completed;
  result.message = std::move(message);
  return result;
}

class BenchmarkAdapter final : public humanoid::adapters::IRobotAdapter {
public:
  humanoid::adapters::Result Initialize() override { return Success(); }
  humanoid::adapters::Result Connect() override { return Success(); }
  humanoid::adapters::Result Disconnect() override { return Success(); }
  humanoid::adapters::Result Shutdown() override { return Success(); }
  humanoid::adapters::Result StandUp() override { return Success(); }
  humanoid::adapters::Result BalanceStand() override { return Success(); }
  humanoid::adapters::Result Move(float, float, float) override { return Success(); }
  humanoid::adapters::Result Stop() override { return Success(); }
  humanoid::adapters::Result EmergencyStop() override { return Success(); }

  [[nodiscard]] humanoid::adapters::RobotStateResult GetRobotState() const override {
    humanoid::adapters::RobotState state{};
    state.vendor = "Benchmark";
    state.model = "Adapter";
    state.connection_state = humanoid::adapters::RobotConnectionState::kConnected;
    state.initialized = true;
    state.connected = true;
    return humanoid::adapters::RobotStateResult{Success(), state};
  }

private:
  [[nodiscard]] static humanoid::adapters::Result Success() {
    return humanoid::adapters::Result{humanoid::adapters::ErrorCode::kSuccess, "ok"};
  }
};

class SuccessNode final : public humanoid::bt::BTNode {
public:
  [[nodiscard]] std::string_view Name() const noexcept override { return "SuccessNode"; }
  [[nodiscard]] humanoid::bt::BTStatus Initialize(humanoid::bt::BTContext&) override {
    return humanoid::bt::BTStatus::Idle;
  }
  [[nodiscard]] humanoid::bt::BTStatus Tick(humanoid::bt::BTContext&) override {
    return humanoid::bt::BTStatus::Success;
  }
  void Reset(humanoid::bt::BTContext&) override {}
  void Shutdown(humanoid::bt::BTContext&) override {}
};

class PassThroughStage final : public humanoid::perception::IPerceptionStage {
public:
  [[nodiscard]] humanoid::common::Status
  Process(humanoid::perception::PerceptionContext& context) override {
    context.metadata["benchmark"] = "true";
    return humanoid::common::Status::ok();
  }
};

std::vector<BenchmarkResult> RunBenchmarks() {
  std::vector<BenchmarkResult> results;
  constexpr std::size_t kFastIterations{1000U};

  humanoid::core::CommandExecutionPipeline pipeline{
      [](const humanoid::core::Command&) { return MakeCompletedCommandResult(); },
      humanoid::core::CommandExecutionPipelineOptions{2048U, 1U, 2048U}};
  results.push_back(
      Measure("Command execution pipeline", kFastIterations, [&pipeline](std::size_t index) {
        auto handle = pipeline.Submit(MakeCommand(
            static_cast<humanoid::core::CommandId>(index + 1U), humanoid::core::CommandType::Stop));
        static_cast<void>(handle.result.get());
      }));
  static_cast<void>(pipeline.Shutdown());

  const auto adapter = std::make_shared<BenchmarkAdapter>();
  const auto dispatcher = std::make_shared<humanoid::core::CommandDispatcher>(adapter);
  humanoid::mission::MissionExecutor mission_executor{dispatcher};
  humanoid::mission::MissionStep step{};
  step.id = 1U;
  step.name = "benchmark-stop";
  step.command = MakeCommand(1U, humanoid::core::CommandType::Stop);
  results.push_back(Measure("Mission step dispatch", kFastIterations,
                            [&mission_executor, &step](std::size_t index) {
                              step.command.id = static_cast<humanoid::core::CommandId>(index + 1U);
                              static_cast<void>(mission_executor.ExecuteStep(step));
                            }));
  static_cast<void>(mission_executor.Stop());
  static_cast<void>(dispatcher->Shutdown());

  humanoid::bt::BehaviorTree tree{std::make_unique<SuccessNode>()};
  static_cast<void>(tree.Initialize());
  results.push_back(Measure("Behavior tree tick", kFastIterations,
                            [&tree](std::size_t) { static_cast<void>(tree.Tick()); }));
  static_cast<void>(tree.Shutdown());

  humanoid::planner::RuleBasedPlanner planner;
  humanoid::planner::PlanningRequest request{};
  request.goal.id = 1U;
  request.goal.type = humanoid::planner::GoalType::Interaction;
  request.goal.description = "Wave to audience";
  results.push_back(
      Measure("Rule-based planner", kFastIterations, [&planner, &request](std::size_t index) {
        request.goal.id = static_cast<humanoid::planner::GoalId>(index + 1U);
        static_cast<void>(planner.Plan(request));
      }));

  const auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
  humanoid::services::TelemetryService telemetry{state_manager, std::chrono::milliseconds{1}};
  std::size_t telemetry_count{0U};
  const auto subscription = telemetry.Subscribe(
      [&telemetry_count](const humanoid::core::RobotState&) { ++telemetry_count; });
  results.push_back(Measure("Telemetry subscription dispatch", kFastIterations,
                            [state_manager](std::size_t index) {
                              humanoid::core::RobotState state{};
                              state.connection.connected = true;
                              state.power.batteryLevel = static_cast<float>(index % 100U);
                              state_manager->UpdateState(state);
                              static_cast<void>(state_manager->GetState());
                            }));
  static_cast<void>(telemetry.Unsubscribe(subscription));
  static_cast<void>(telemetry_count);

  humanoid::perception::PerceptionPipeline perception;
  humanoid::perception::PerceptionStageDescriptor descriptor{};
  descriptor.stageId = "pass";
  descriptor.name = "Pass Through";
  descriptor.type = humanoid::perception::PerceptionStageType::Output;
  static_cast<void>(perception.RegisterStage(descriptor, std::make_shared<PassThroughStage>()));
  humanoid::perception::SensorFrame frame{};
  frame.id = 1U;
  frame.metadata["sensor_id"] = "benchmark-camera";
  results.push_back(
      Measure("Perception pipeline", kFastIterations, [&perception, &frame](std::size_t) {
        static_cast<void>(perception.Execute(frame));
      }));

  humanoid::plugins::PluginRegistry plugin_registry;
  humanoid::plugins::PluginMetadata metadata{};
  metadata.plugin_id = "org.humanoid.benchmark";
  metadata.name = "Benchmark Plugin";
  metadata.vendor = "humanoid-core";
  metadata.description = "Benchmark registry record";
  static_cast<void>(plugin_registry.RegisterPlugin(metadata));
  results.push_back(
      Measure("Plugin registry lookup", kFastIterations, [&plugin_registry](std::size_t) {
        static_cast<void>(plugin_registry.Contains("org.humanoid.benchmark"));
      }));

  humanoid::runtime::RuntimeScheduler scheduler{
      humanoid::runtime::RuntimeSchedulerOptions{2048U, 1U, 2048U}};
  results.push_back(Measure("Runtime scheduler", kFastIterations, [&scheduler](std::size_t index) {
    humanoid::runtime::RuntimeJob job{};
    job.id = static_cast<humanoid::runtime::RuntimeJobId>(index + 1U);
    job.callback = [](humanoid::runtime::RuntimeJobContext&) {
      return humanoid::runtime::RuntimeJobResult{humanoid::runtime::ExecutionState::Completed,
                                                 "ok"};
    };
    auto handle = scheduler.Submit(std::move(job));
    static_cast<void>(handle.result.get());
  }));
  scheduler.Shutdown();

  humanoid::cloud::CloudPlatform cloud;
  static_cast<void>(cloud.Start());
  results.push_back(Measure("Cloud API catalog", kFastIterations, [&cloud](std::size_t) {
    static_cast<void>(cloud.RestApi()->FindBySubsystem("Robot"));
  }));
  results.push_back(Measure("Cloud fleet heartbeat", kFastIterations, [&cloud](std::size_t index) {
    const std::string robot_id = "robot-" + std::to_string(index + 1U);
    humanoid::cloud::fleet::RobotRecord robot{};
    robot.robotId = robot_id;
    robot.vendor = "Benchmark";
    robot.model = "Cloud";
    static_cast<void>(cloud.Fleet()->RegisterRobot(robot));
    static_cast<void>(cloud.Fleet()->Heartbeat(robot_id, true));
  }));
  cloud.Stop();

  return results;
}

} // namespace

int main() {
  const auto results = RunBenchmarks();

  std::cout << std::left << std::setw(34) << "Benchmark" << std::right << std::setw(12)
            << "Iterations" << std::setw(14) << "Total ms" << std::setw(18) << "Avg us/op" << '\n';
  for (const auto& result : results) {
    std::cout << std::left << std::setw(34) << result.name << std::right << std::setw(12)
              << result.iterations << std::setw(14) << std::fixed << std::setprecision(3)
              << result.totalMilliseconds << std::setw(18) << std::fixed << std::setprecision(3)
              << result.averageMicroseconds << '\n';
  }

  return EXIT_SUCCESS;
}
