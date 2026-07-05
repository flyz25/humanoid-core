/**
 * @file bt_decorator_nodes_unit_test.cpp
 * @brief Validates vendor-independent behavior tree decorator nodes.
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
#include <humanoid/bt/FailerNode.h>
#include <humanoid/bt/InverterNode.h>
#include <humanoid/bt/LimiterNode.h>
#include <humanoid/bt/RepeatNode.h>
#include <humanoid/bt/RetryNode.h>
#include <humanoid/bt/SucceederNode.h>
#include <humanoid/bt/TimeoutNode.h>
#include <humanoid/runtime/ExecutionContext.h>

namespace {

using humanoid::bt::BTContext;
using humanoid::bt::BTNode;
using humanoid::bt::BTStatus;
using humanoid::bt::FailerNode;
using humanoid::bt::InverterNode;
using humanoid::bt::LimiterNode;
using humanoid::bt::RepeatNode;
using humanoid::bt::RetryNode;
using humanoid::bt::SucceederNode;
using humanoid::bt::TimeoutNode;
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
  ScriptedNode(std::shared_ptr<std::atomic<int>> ticks, std::vector<BTStatus> script,
               std::chrono::milliseconds sleep = std::chrono::milliseconds{0})
      : ticks_(std::move(ticks)), script_(std::move(script)), sleep_(sleep) {
    if (script_.empty()) {
      script_.push_back(BTStatus::Success);
    }
  }

  [[nodiscard]] std::string_view Name() const noexcept override { return "ScriptedNode"; }
  [[nodiscard]] BTStatus Initialize(BTContext&) override { return BTStatus::Idle; }

  [[nodiscard]] BTStatus Tick(BTContext&) override {
    if (sleep_ > std::chrono::milliseconds{0}) {
      std::this_thread::sleep_for(sleep_);
    }
    const int tick = ++(*ticks_);
    const std::size_t index =
        static_cast<std::size_t>(std::min(tick - 1, static_cast<int>(script_.size() - 1U)));
    return script_[index];
  }

  void Reset(BTContext&) override { ++reset_count_; }
  void Shutdown(BTContext&) override {}

  [[nodiscard]] int ResetCount() const noexcept { return reset_count_.load(); }

private:
  std::shared_ptr<std::atomic<int>> ticks_;
  std::vector<BTStatus> script_;
  std::chrono::milliseconds sleep_;
  std::atomic<int> reset_count_{0};
};

std::unique_ptr<ScriptedNode> MakeScripted(std::shared_ptr<std::atomic<int>> ticks,
                                           std::vector<BTStatus> script) {
  return std::make_unique<ScriptedNode>(std::move(ticks), std::move(script));
}

void TestInverterSucceederFailer() {
  BTContext context;
  auto inverter_ticks = std::make_shared<std::atomic<int>>(0);
  InverterNode inverter{"Inverter", MakeScripted(inverter_ticks, {BTStatus::Success})};
  Check(inverter.Initialize(context) == BTStatus::Idle, "inverter initialization failed");
  Check(inverter.Tick(context) == BTStatus::Failure, "inverter did not invert success");

  auto succeeder_ticks = std::make_shared<std::atomic<int>>(0);
  SucceederNode succeeder{"Succeeder", MakeScripted(succeeder_ticks, {BTStatus::Failure})};
  Check(succeeder.Initialize(context) == BTStatus::Idle, "succeeder initialization failed");
  Check(succeeder.Tick(context) == BTStatus::Success, "succeeder did not force success");

  auto failer_ticks = std::make_shared<std::atomic<int>>(0);
  FailerNode failer{"Failer", MakeScripted(failer_ticks, {BTStatus::Success})};
  Check(failer.Initialize(context) == BTStatus::Idle, "failer initialization failed");
  Check(failer.Tick(context) == BTStatus::Failure, "failer did not force failure");
}

void TestRepeatLoop() {
  BTContext context;
  auto ticks = std::make_shared<std::atomic<int>>(0);
  RepeatNode repeat{"RepeatThree", 3U, MakeScripted(ticks, {BTStatus::Success})};
  Check(repeat.Initialize(context) == BTStatus::Idle, "repeat initialization failed");
  Check(repeat.Tick(context) == BTStatus::Running, "repeat first tick did not continue");
  Check(repeat.Tick(context) == BTStatus::Running, "repeat second tick did not continue");
  Check(repeat.Tick(context) == BTStatus::Success, "repeat third tick did not finish");
  Check(ticks->load() == 3, "repeat did not tick child expected number of times");
}

void TestRetry() {
  BTContext context;
  auto ticks = std::make_shared<std::atomic<int>>(0);
  RetryNode retry{"RetryThree", 3U,
                  MakeScripted(ticks, {BTStatus::Failure, BTStatus::Failure, BTStatus::Success})};
  Check(retry.Initialize(context) == BTStatus::Idle, "retry initialization failed");
  Check(retry.Tick(context) == BTStatus::Running, "retry first failure did not continue");
  Check(retry.Tick(context) == BTStatus::Running, "retry second failure did not continue");
  Check(retry.Tick(context) == BTStatus::Success, "retry did not succeed on third attempt");
  Check(ticks->load() == 3, "retry did not tick child expected number of times");

  auto failing_ticks = std::make_shared<std::atomic<int>>(0);
  RetryNode failing_retry{"RetryTwo", 2U,
                          MakeScripted(failing_ticks, {BTStatus::Failure, BTStatus::Failure})};
  Check(failing_retry.Initialize(context) == BTStatus::Idle, "failing retry initialization failed");
  Check(failing_retry.Tick(context) == BTStatus::Running, "failing retry first tick mismatch");
  Check(failing_retry.Tick(context) == BTStatus::Failure, "failing retry did not fail");
}

void TestLimiter() {
  BTContext context;
  auto ticks = std::make_shared<std::atomic<int>>(0);
  LimiterNode limiter{"LimiterTwo", 2U, MakeScripted(ticks, {BTStatus::Success})};
  Check(limiter.Initialize(context) == BTStatus::Idle, "limiter initialization failed");
  Check(limiter.Tick(context) == BTStatus::Success, "limiter first tick failed");
  Check(limiter.Tick(context) == BTStatus::Success, "limiter second tick failed");
  Check(limiter.Tick(context) == BTStatus::Failure, "limiter did not block third tick");
  Check(ticks->load() == 2, "limiter ticked child after capacity was exhausted");
  limiter.Reset(context);
  Check(limiter.Tick(context) == BTStatus::Success, "limiter reset did not restore capacity");
}

void TestTimeout() {
  BTContext context;
  auto ticks = std::make_shared<std::atomic<int>>(0);
  TimeoutNode timeout{"Timeout", std::chrono::milliseconds{20},
                      MakeScripted(ticks, {BTStatus::Running})};
  Check(timeout.Initialize(context) == BTStatus::Idle, "timeout initialization failed");
  Check(timeout.Tick(context) == BTStatus::Running, "timeout first tick did not run");
  std::this_thread::sleep_for(std::chrono::milliseconds{25});
  Check(timeout.Tick(context) == BTStatus::Failure, "timeout did not fail after deadline");

  auto slow_ticks = std::make_shared<std::atomic<int>>(0);
  TimeoutNode blocking_timeout{
      "BlockingTimeout", std::chrono::milliseconds{10},
      std::make_unique<ScriptedNode>(slow_ticks, std::vector<BTStatus>{BTStatus::Running},
                                     std::chrono::milliseconds{20})};
  Check(blocking_timeout.Initialize(context) == BTStatus::Idle,
        "blocking timeout initialization failed");
  Check(blocking_timeout.Tick(context) == BTStatus::Failure,
        "blocking timeout did not fail after long child tick");
}

void TestCancellationAbortsDecorator() {
  auto execution =
      std::make_shared<ExecutionContext>(ExecutionContextId{8300U}, ExecutionScope::BehaviorTree);
  BTContext context{execution, std::make_shared<humanoid::runtime::Blackboard>()};
  auto ticks = std::make_shared<std::atomic<int>>(0);
  InverterNode inverter{"CancelledInverter", MakeScripted(ticks, {BTStatus::Success})};
  Check(inverter.Initialize(context) == BTStatus::Idle, "cancelled inverter init failed");
  Check(execution->RequestCancellation(), "failed to cancel execution");
  Check(inverter.Tick(context) == BTStatus::Aborted, "cancelled decorator did not abort");
  Check(ticks->load() == 0, "cancelled decorator ticked child");
}

} // namespace

int main() {
  try {
    TestInverterSucceederFailer();
    TestRepeatLoop();
    TestRetry();
    TestLimiter();
    TestTimeout();
    TestCancellationAbortsDecorator();
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    (void)exception;
    return EXIT_FAILURE;
  }
}
