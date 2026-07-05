/**
 * @file bt_composite_nodes_unit_test.cpp
 * @brief Validates vendor-independent behavior tree composite nodes.
 */

#include <algorithm>
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
#include <humanoid/bt/ParallelNode.h>
#include <humanoid/bt/SelectorNode.h>
#include <humanoid/bt/SequenceNode.h>
#include <humanoid/runtime/ExecutionContext.h>

namespace {

using humanoid::bt::BTChildren;
using humanoid::bt::BTContext;
using humanoid::bt::BTNode;
using humanoid::bt::BTStatus;
using humanoid::bt::ParallelNode;
using humanoid::bt::SelectorMemoryPolicy;
using humanoid::bt::SelectorNode;
using humanoid::bt::SequenceMemoryPolicy;
using humanoid::bt::SequenceNode;
using humanoid::runtime::ExecutionContext;
using humanoid::runtime::ExecutionContextId;
using humanoid::runtime::ExecutionScope;

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

class ScriptedNode final : public BTNode {
public:
  ScriptedNode(std::shared_ptr<std::atomic<int>> ticks, std::vector<BTStatus> script)
      : ticks_(std::move(ticks)), script_(std::move(script)) {
    if (script_.empty()) {
      script_.push_back(BTStatus::Success);
    }
  }

  [[nodiscard]] std::string_view Name() const noexcept override { return "ScriptedNode"; }
  [[nodiscard]] BTStatus Initialize(BTContext&) override { return BTStatus::Idle; }

  [[nodiscard]] BTStatus Tick(BTContext&) override {
    const int tick = ++(*ticks_);
    const std::size_t index =
        static_cast<std::size_t>(std::min(tick - 1, static_cast<int>(script_.size() - 1U)));
    return script_[index];
  }

  void Reset(BTContext&) override {}
  void Shutdown(BTContext&) override {}

private:
  std::shared_ptr<std::atomic<int>> ticks_;
  std::vector<BTStatus> script_;
};

class ConcurrentNode final : public BTNode {
public:
  ConcurrentNode(std::shared_ptr<std::atomic<int>> active,
                 std::shared_ptr<std::atomic<int>> maximum_active)
      : active_(std::move(active)), maximum_active_(std::move(maximum_active)) {}

  [[nodiscard]] std::string_view Name() const noexcept override { return "ConcurrentNode"; }
  [[nodiscard]] BTStatus Initialize(BTContext&) override { return BTStatus::Idle; }

  [[nodiscard]] BTStatus Tick(BTContext&) override {
    const int current = ++(*active_);
    int observed = maximum_active_->load();
    while (current > observed && !maximum_active_->compare_exchange_weak(observed, current)) {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{25});
    --(*active_);
    return BTStatus::Success;
  }

  void Reset(BTContext&) override {}
  void Shutdown(BTContext&) override {}

private:
  std::shared_ptr<std::atomic<int>> active_;
  std::shared_ptr<std::atomic<int>> maximum_active_;
};

std::unique_ptr<BTNode> MakeScripted(std::shared_ptr<std::atomic<int>> ticks,
                                     std::vector<BTStatus> script) {
  return std::make_unique<ScriptedNode>(std::move(ticks), std::move(script));
}

void TestSequenceAndSelector() {
  BTContext context;
  auto first = std::make_shared<std::atomic<int>>(0);
  auto second = std::make_shared<std::atomic<int>>(0);
  SequenceNode sequence{"Sequence", SequenceMemoryPolicy::Stateless};
  Check(sequence.AddChild(MakeScripted(first, {BTStatus::Success})), "failed to add first child");
  Check(sequence.AddChild(MakeScripted(second, {BTStatus::Success})), "failed to add second child");
  Check(sequence.ChildCount() == 2U, "sequence child count mismatch");
  Check(sequence.Initialize(context) == BTStatus::Idle, "sequence initialization failed");
  Check(sequence.Tick(context) == BTStatus::Success, "sequence did not succeed");

  auto failed = std::make_shared<std::atomic<int>>(0);
  auto selected = std::make_shared<std::atomic<int>>(0);
  SelectorNode selector{"Selector", SelectorMemoryPolicy::Stateless};
  Check(selector.AddChild(MakeScripted(failed, {BTStatus::Failure})),
        "failed to add failed selector child");
  Check(selector.AddChild(MakeScripted(selected, {BTStatus::Success})),
        "failed to add selected child");
  Check(selector.Initialize(context) == BTStatus::Idle, "selector initialization failed");
  Check(selector.Tick(context) == BTStatus::Success, "selector did not succeed");
  Check(failed->load() == 1 && selected->load() == 1, "selector ticked unexpected children");
}

void TestMemorySequenceAndSelector() {
  BTContext context;
  auto first = std::make_shared<std::atomic<int>>(0);
  auto running = std::make_shared<std::atomic<int>>(0);
  SequenceNode memory_sequence{"MemorySequence", SequenceMemoryPolicy::Memory};
  Check(memory_sequence.AddChild(MakeScripted(first, {BTStatus::Success})),
        "failed to add memory sequence first child");
  Check(memory_sequence.AddChild(MakeScripted(running, {BTStatus::Running, BTStatus::Success})),
        "failed to add memory sequence running child");
  Check(memory_sequence.Initialize(context) == BTStatus::Idle, "memory sequence init failed");
  Check(memory_sequence.Tick(context) == BTStatus::Running, "memory sequence did not run");
  Check(memory_sequence.Tick(context) == BTStatus::Success, "memory sequence did not resume");
  Check(first->load() == 1, "memory sequence restarted before running child");
  Check(running->load() == 2, "memory sequence running child count mismatch");

  auto failing = std::make_shared<std::atomic<int>>(0);
  auto selected = std::make_shared<std::atomic<int>>(0);
  SelectorNode memory_selector{"MemorySelector", SelectorMemoryPolicy::Memory};
  Check(memory_selector.AddChild(MakeScripted(failing, {BTStatus::Failure})),
        "failed to add memory selector failed child");
  Check(memory_selector.AddChild(MakeScripted(selected, {BTStatus::Running, BTStatus::Success})),
        "failed to add memory selector selected child");
  Check(memory_selector.Initialize(context) == BTStatus::Idle, "memory selector init failed");
  Check(memory_selector.Tick(context) == BTStatus::Running, "memory selector did not run");
  Check(memory_selector.Tick(context) == BTStatus::Success, "memory selector did not resume");
  Check(failing->load() == 1, "memory selector restarted before running child");
  Check(selected->load() == 2, "memory selector selected child count mismatch");
}

void TestNestedComposites() {
  BTContext context;
  auto selector_failure = std::make_shared<std::atomic<int>>(0);
  auto selector_success = std::make_shared<std::atomic<int>>(0);
  BTChildren selector_children;
  selector_children.push_back(MakeScripted(selector_failure, {BTStatus::Failure}));
  selector_children.push_back(MakeScripted(selector_success, {BTStatus::Success}));

  auto sequence_tail = std::make_shared<std::atomic<int>>(0);
  SequenceNode root{"NestedSequence", SequenceMemoryPolicy::Stateless};
  Check(root.AddChild(std::make_unique<SelectorNode>(
            "NestedSelector", SelectorMemoryPolicy::Stateless, std::move(selector_children))),
        "failed to add nested selector");
  Check(root.AddChild(MakeScripted(sequence_tail, {BTStatus::Success})),
        "failed to add nested sequence tail");

  Check(root.Initialize(context) == BTStatus::Idle, "nested tree initialization failed");
  Check(root.Tick(context) == BTStatus::Success, "nested composite did not succeed");
  Check(selector_failure->load() == 1 && selector_success->load() == 1 &&
            sequence_tail->load() == 1,
        "nested composite tick counts mismatch");
}

void TestParallelExecution() {
  BTContext context;
  auto active = std::make_shared<std::atomic<int>>(0);
  auto maximum_active = std::make_shared<std::atomic<int>>(0);
  ParallelNode parallel{"Parallel", 0U, 1U};
  Check(parallel.AddChild(std::make_unique<ConcurrentNode>(active, maximum_active)),
        "failed to add first parallel child");
  Check(parallel.AddChild(std::make_unique<ConcurrentNode>(active, maximum_active)),
        "failed to add second parallel child");
  Check(parallel.AddChild(std::make_unique<ConcurrentNode>(active, maximum_active)),
        "failed to add third parallel child");
  Check(parallel.Initialize(context) == BTStatus::Idle, "parallel initialization failed");
  Check(parallel.Tick(context) == BTStatus::Success, "parallel node did not succeed");
  Check(maximum_active->load() > 1, "parallel node did not tick children concurrently");
}

void TestCancellationAbortsComposite() {
  auto execution =
      std::make_shared<ExecutionContext>(ExecutionContextId{8200U}, ExecutionScope::BehaviorTree);
  BTContext context{execution, std::make_shared<humanoid::runtime::Blackboard>()};
  SequenceNode sequence{"CancelledSequence", SequenceMemoryPolicy::Stateless};
  Check(sequence.AddChild(MakeScripted(std::make_shared<std::atomic<int>>(0), {BTStatus::Success})),
        "failed to add cancelled sequence child");
  Check(sequence.Initialize(context) == BTStatus::Idle, "cancelled sequence init failed");
  Check(execution->RequestCancellation(), "failed to cancel execution");
  Check(sequence.Tick(context) == BTStatus::Aborted, "cancelled sequence did not abort");
}

} // namespace

int main() {
  try {
    TestSequenceAndSelector();
    TestMemorySequenceAndSelector();
    TestNestedComposites();
    TestParallelExecution();
    TestCancellationAbortsComposite();
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    (void)exception;
    return EXIT_FAILURE;
  }
}
