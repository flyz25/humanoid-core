/**
 * @file bt_runtime_integration_test.cpp
 * @brief Validates behavior tree execution through shared runtime services.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <humanoid/bt/BTContext.h>
#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BTStatus.h>
#include <humanoid/bt/BehaviorTree.h>
#include <humanoid/bt/BehaviorTreeFactory.h>
#include <humanoid/bt/BehaviorTreeRuntime.h>
#include <humanoid/runtime/Blackboard.h>
#include <humanoid/runtime/ExecutionScope.h>
#include <humanoid/runtime/ExecutionState.h>
#include <humanoid/runtime/ResourceManager.h>
#include <humanoid/runtime/RuntimeScheduler.h>

namespace {

using humanoid::bt::BehaviorTree;
using humanoid::bt::BehaviorTreeFactory;
using humanoid::bt::BehaviorTreeJobOptions;
using humanoid::bt::BehaviorTreeRuntime;
using humanoid::bt::BTContext;
using humanoid::bt::BTNode;
using humanoid::bt::BTStatus;
using humanoid::runtime::Blackboard;
using humanoid::runtime::ExecutionScope;
using humanoid::runtime::ExecutionState;
using humanoid::runtime::ResourceManager;
using humanoid::runtime::RuntimeJobResult;
using humanoid::runtime::RuntimeScheduler;
using humanoid::runtime::RuntimeSchedulerOptions;

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

struct ProbeState final {
  std::atomic<int> active{0};
  std::atomic<int> maximumActive{0};
  std::atomic<int> ticks{0};
  std::atomic<bool> contextMismatch{false};
  std::atomic<bool> blackboardMismatch{false};
  std::mutex idsMutex;
  std::vector<std::uint64_t> executionIds;
};

class RuntimeProbeNode final : public BTNode {
public:
  RuntimeProbeNode(std::shared_ptr<ProbeState> state, std::shared_ptr<Blackboard> blackboard,
                   std::chrono::milliseconds work_duration, bool remain_running)
      : state_(std::move(state)), blackboard_(std::move(blackboard)), work_duration_(work_duration),
        remain_running_(remain_running) {}

  [[nodiscard]] std::string_view Name() const noexcept override { return "RuntimeProbe"; }

  [[nodiscard]] BTStatus Initialize(BTContext&) override { return BTStatus::Idle; }

  [[nodiscard]] BTStatus Tick(BTContext& context) override {
    ++state_->ticks;
    const std::shared_ptr<humanoid::runtime::ExecutionContext> execution = context.Execution();
    if (execution->Scope() != ExecutionScope::BehaviorTree || execution->Id() == 0U) {
      state_->contextMismatch = true;
    }
    if (context.Blackboard() != blackboard_) {
      state_->blackboardMismatch = true;
    }
    {
      std::lock_guard<std::mutex> lock{state_->idsMutex};
      state_->executionIds.push_back(execution->Id());
    }
    static_cast<void>(context.Blackboard()->Store("bt-runtime", std::to_string(execution->Id()),
                                                  execution->Id()));

    if (remain_running_) {
      return BTStatus::Running;
    }

    const int active = ++state_->active;
    int maximum = state_->maximumActive.load();
    while (active > maximum && !state_->maximumActive.compare_exchange_weak(maximum, active)) {
    }
    std::this_thread::sleep_for(work_duration_);
    --state_->active;
    return BTStatus::Success;
  }

  void Reset(BTContext&) override {}
  void Shutdown(BTContext&) override {}

private:
  std::shared_ptr<ProbeState> state_;
  std::shared_ptr<Blackboard> blackboard_;
  std::chrono::milliseconds work_duration_;
  bool remain_running_{false};
};

[[nodiscard]] std::shared_ptr<BehaviorTreeFactory>
MakeFactory(const std::shared_ptr<ProbeState>& state, const std::shared_ptr<Blackboard>& blackboard,
            std::chrono::milliseconds work_duration, bool remain_running = false) {
  auto factory = std::make_shared<BehaviorTreeFactory>();
  Check(factory->RegisterNode("RuntimeProbe",
                              [state, blackboard, work_duration, remain_running]() {
                                return std::make_unique<RuntimeProbeNode>(
                                    state, blackboard, work_duration, remain_running);
                              }),
        "failed to register runtime probe node");
  return factory;
}

void TestConcurrentTreesAndMultipleRuntimeInstances() {
  auto scheduler = std::make_shared<RuntimeScheduler>(RuntimeSchedulerOptions{16U, 4U, 16U});
  auto blackboard = std::make_shared<Blackboard>();
  auto resources = std::make_shared<ResourceManager>();
  auto state = std::make_shared<ProbeState>();
  auto factory = MakeFactory(state, blackboard, std::chrono::milliseconds{40});
  BehaviorTreeRuntime first_runtime{scheduler, blackboard, resources, factory};
  BehaviorTreeRuntime second_runtime{scheduler, blackboard, resources, factory};

  BehaviorTreeJobOptions first_options;
  first_options.executionId = 8101U;
  BehaviorTreeJobOptions second_options;
  second_options.executionId = 8102U;
  auto first = first_runtime.Submit("RuntimeProbe", std::move(first_options));
  auto second = second_runtime.Submit("RuntimeProbe", std::move(second_options));

  Check(first.result.get().state == ExecutionState::Completed, "first runtime tree failed");
  Check(second.result.get().state == ExecutionState::Completed, "second runtime tree failed");
  Check(state->maximumActive.load() > 1, "independent runtime trees did not run concurrently");
  Check(!state->contextMismatch.load(), "tree did not receive scheduler execution context");
  Check(!state->blackboardMismatch.load(), "tree did not receive shared runtime blackboard");
  Check(blackboard->Contains("bt-runtime", "8101") && blackboard->Contains("bt-runtime", "8102"),
        "concurrent trees did not publish into the shared blackboard");
}

void TestExclusiveResourceAcrossRuntimeInstances() {
  auto scheduler = std::make_shared<RuntimeScheduler>(RuntimeSchedulerOptions{16U, 2U, 16U});
  auto blackboard = std::make_shared<Blackboard>();
  auto resources = std::make_shared<ResourceManager>();
  auto state = std::make_shared<ProbeState>();
  auto factory = MakeFactory(state, blackboard, std::chrono::milliseconds{30});
  BehaviorTreeRuntime first_runtime{scheduler, blackboard, resources, factory};
  BehaviorTreeRuntime second_runtime{scheduler, blackboard, resources, factory};

  BehaviorTreeJobOptions first_options;
  first_options.executionId = 8201U;
  first_options.resourceId = "robot/main";
  first_options.resourceTimeout = std::chrono::milliseconds{200};
  BehaviorTreeJobOptions second_options = first_options;
  second_options.executionId = 8202U;

  auto first = first_runtime.Submit("RuntimeProbe", std::move(first_options));
  auto second = second_runtime.Submit("RuntimeProbe", std::move(second_options));

  Check(first.result.get().state == ExecutionState::Completed, "first resource tree failed");
  Check(second.result.get().state == ExecutionState::Completed, "second resource tree failed");
  Check(state->maximumActive.load() == 1, "exclusive behavior tree resource overlapped");
  Check(resources->ActiveLockCount("robot/main") == 0U, "resource lease was not released");
}

void TestConstructedTreeAndCancellation() {
  auto scheduler = std::make_shared<RuntimeScheduler>(RuntimeSchedulerOptions{8U, 2U, 8U});
  auto blackboard = std::make_shared<Blackboard>();
  auto resources = std::make_shared<ResourceManager>();
  auto state = std::make_shared<ProbeState>();
  auto factory = MakeFactory(state, blackboard, std::chrono::milliseconds::zero(), true);
  BehaviorTreeRuntime runtime{scheduler, blackboard, resources, factory};

  auto tree = std::make_unique<BehaviorTree>(std::make_unique<RuntimeProbeNode>(
      state, blackboard, std::chrono::milliseconds::zero(), true));
  BehaviorTreeJobOptions options;
  options.executionId = 8301U;
  options.tickInterval = std::chrono::milliseconds{500};
  auto handle = runtime.Submit(std::move(tree), std::move(options));

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
  while (state->ticks.load() == 0 && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }
  Check(state->ticks.load() > 0, "constructed tree did not start");
  Check(scheduler->Stop(8301U), "runtime tree cancellation request failed");
  const RuntimeJobResult result = handle.result.get();
  Check(result.state == ExecutionState::Cancelled, "cancelled runtime tree did not stop");
}

} // namespace

int main() {
  try {
    TestConcurrentTreesAndMultipleRuntimeInstances();
    TestExclusiveResourceAcrossRuntimeInstances();
    TestConstructedTreeAndCancellation();
    return EXIT_SUCCESS;
  } catch (...) {
    return EXIT_FAILURE;
  }
}
