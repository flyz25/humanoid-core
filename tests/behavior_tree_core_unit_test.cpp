/**
 * @file behavior_tree_core_unit_test.cpp
 * @brief Validates the vendor-independent behavior tree core.
 */

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <humanoid/bt/BTContext.h>
#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BTStatus.h>
#include <humanoid/bt/BehaviorTree.h>
#include <humanoid/bt/BehaviorTreeFactory.h>
#include <humanoid/runtime/ExecutionContext.h>
#include <humanoid/runtime/ExecutionState.h>

namespace {

using humanoid::bt::BehaviorTree;
using humanoid::bt::BehaviorTreeFactory;
using humanoid::bt::BTContext;
using humanoid::bt::BTNode;
using humanoid::bt::BTStatus;
using humanoid::runtime::ExecutionContext;
using humanoid::runtime::ExecutionContextId;
using humanoid::runtime::ExecutionScope;
using humanoid::runtime::ExecutionState;

struct ProbeState final {
  std::atomic<int> initialize_count{0};
  std::atomic<int> tick_count{0};
  std::atomic<int> reset_count{0};
  std::atomic<int> shutdown_count{0};
};

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

class ScriptedNode final : public BTNode {
public:
  ScriptedNode(std::shared_ptr<ProbeState> state, std::vector<BTStatus> script)
      : state_(std::move(state)), script_(std::move(script)) {}

  [[nodiscard]] std::string_view Name() const noexcept override { return "ScriptedNode"; }

  [[nodiscard]] BTStatus Initialize(BTContext&) override {
    ++state_->initialize_count;
    return BTStatus::Idle;
  }

  [[nodiscard]] BTStatus Tick(BTContext& context) override {
    const int tick = ++state_->tick_count;
    const auto blackboard = context.Blackboard();
    (void)blackboard->Store("bt", "tick_count", tick);
    const std::size_t index =
        static_cast<std::size_t>(std::min(tick - 1, static_cast<int>(script_.size() - 1U)));
    return script_[index];
  }

  void Reset(BTContext&) override { ++state_->reset_count; }

  void Shutdown(BTContext&) override { ++state_->shutdown_count; }

private:
  std::shared_ptr<ProbeState> state_;
  std::vector<BTStatus> script_;
};

class ThrowingCreatorNode final : public BTNode {
public:
  [[nodiscard]] std::string_view Name() const noexcept override { return "ThrowingCreatorNode"; }
  [[nodiscard]] BTStatus Initialize(BTContext&) override { return BTStatus::Idle; }
  [[nodiscard]] BTStatus Tick(BTContext&) override { return BTStatus::Success; }
  void Reset(BTContext&) override {}
  void Shutdown(BTContext&) override {}
};

void TestStatusHelpers() {
  Check(humanoid::bt::toString(BTStatus::Running) == "Running", "status name mismatch");
  Check(!humanoid::bt::isTerminal(BTStatus::Running), "running status is terminal");
  Check(humanoid::bt::isTerminal(BTStatus::Success), "success status is not terminal");
}

void TestTreeLifecycle() {
  auto execution =
      std::make_shared<ExecutionContext>(ExecutionContextId{81U}, ExecutionScope::BehaviorTree);
  auto blackboard = std::make_shared<humanoid::runtime::Blackboard>();
  auto state = std::make_shared<ProbeState>();

  BehaviorTree tree{std::make_unique<ScriptedNode>(
                        state, std::vector<BTStatus>{BTStatus::Running, BTStatus::Success}),
                    BTContext{execution, blackboard}};

  Check(tree.HasRoot(), "tree did not retain root node");
  Check(tree.Status() == BTStatus::Idle, "new tree was not idle");
  Check(tree.Initialize() == BTStatus::Idle, "tree initialization failed");
  Check(state->initialize_count.load() == 1, "node initialized unexpected number of times");
  Check(execution->StartTimestamp().has_value(), "execution start timestamp was not set");

  Check(tree.Tick() == BTStatus::Running, "first tick did not run");
  Check(execution->State() == ExecutionState::Running, "runtime state did not enter running");
  Check(tree.Tick() == BTStatus::Success, "second tick did not succeed");
  Check(execution->State() == ExecutionState::Completed, "runtime state did not complete");
  Check(tree.Tick() == BTStatus::Success, "terminal tick did not remain stable");
  Check(state->tick_count.load() == 2, "terminal tree ticked root without reset");

  const std::shared_ptr<const int> tick_count = blackboard->Get<int>("bt", "tick_count");
  Check(tick_count && *tick_count == 2, "blackboard did not receive tick count");

  Check(tree.Reset(), "tree reset failed");
  Check(tree.Status() == BTStatus::Idle, "tree reset did not return to idle");
  Check(state->reset_count.load() == 1, "node reset count mismatch");
  Check(tree.Shutdown(), "tree shutdown failed");
  Check(!tree.Shutdown(), "tree shutdown was not idempotent");
  Check(state->shutdown_count.load() == 1, "node shutdown count mismatch");
}

void TestCancellationAbortsTick() {
  auto execution =
      std::make_shared<ExecutionContext>(ExecutionContextId{82U}, ExecutionScope::BehaviorTree);
  auto state = std::make_shared<ProbeState>();
  BehaviorTree tree{std::make_unique<ScriptedNode>(state, std::vector<BTStatus>{BTStatus::Failure}),
                    BTContext{execution, std::make_shared<humanoid::runtime::Blackboard>()}};

  Check(tree.Initialize() == BTStatus::Idle, "cancellation tree initialization failed");
  Check(execution->RequestCancellation(), "execution cancellation request failed");
  Check(tree.Tick() == BTStatus::Aborted, "cancelled tree did not abort");
  Check(state->tick_count.load() == 0, "cancelled tree ticked root node");
  Check(execution->State() == ExecutionState::Aborted, "runtime state did not abort");
}

void TestFactory() {
  BehaviorTreeFactory factory;
  auto state = std::make_shared<ProbeState>();

  Check(!factory.RegisterNode("", [] { return std::make_unique<ThrowingCreatorNode>(); }),
        "factory accepted empty node type");
  Check(factory.RegisterNode("Scripted",
                             [state] {
                               return std::make_unique<ScriptedNode>(
                                   state, std::vector<BTStatus>{BTStatus::Success});
                             }),
        "factory registration failed");
  Check(!factory.RegisterNode("Scripted", [] { return std::make_unique<ThrowingCreatorNode>(); }),
        "factory accepted duplicate registration");
  Check(factory.Contains("Scripted"), "factory does not contain registered type");
  Check(factory.Size() == 1U, "factory size mismatch");
  Check(factory.RegisterNode(
            "Throws",
            []() -> std::unique_ptr<BTNode> { throw std::runtime_error{"creator failure"}; }),
        "throwing creator registration failed");

  std::unique_ptr<BTNode> node = factory.CreateNode("Scripted");
  Check(node != nullptr, "factory did not create registered node");
  Check(!factory.CreateNode("Missing"), "factory created missing node");
  Check(!factory.CreateNode("Throws"), "factory did not contain creator exception");

  std::unique_ptr<BehaviorTree> tree = factory.CreateTree("Scripted");
  Check(tree != nullptr, "factory did not create tree");
  Check(tree->Tick() == BTStatus::Success, "factory-created tree did not tick");

  const std::vector<std::string> node_types = factory.RegisteredNodeTypes();
  Check(node_types.size() == 2U && node_types[0] == "Scripted" && node_types[1] == "Throws",
        "factory enumeration mismatch");
  Check(factory.UnregisterNode("Scripted"), "factory unregister failed");
  Check(!factory.Contains("Scripted"), "factory still contains unregistered node");
}

void TestConcurrentTicksAreSerialized() {
  auto state = std::make_shared<ProbeState>();
  BehaviorTree tree{std::make_unique<ScriptedNode>(
      state, std::vector<BTStatus>{BTStatus::Running, BTStatus::Running, BTStatus::Success})};

  std::vector<std::jthread> workers;
  workers.reserve(8U);
  for (int index = 0; index < 8; ++index) {
    workers.emplace_back([&tree] {
      for (int tick = 0; tick < 8; ++tick) {
        const BTStatus status = tree.Tick();
        Check(status == BTStatus::Running || status == BTStatus::Success,
              "unexpected concurrent tick status");
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
      }
    });
  }
  workers.clear();

  Check(tree.Status() == BTStatus::Success, "concurrent tree did not finish successfully");
  Check(state->tick_count.load() == 3, "terminal concurrent tree ticked too many times");
}

} // namespace

int main() {
  try {
    TestStatusHelpers();
    TestTreeLifecycle();
    TestCancellationAbortsTick();
    TestFactory();
    TestConcurrentTicksAreSerialized();
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    (void)exception;
    return EXIT_FAILURE;
  }
}
