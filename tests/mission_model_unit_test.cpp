/**
 * @file mission_model_unit_test.cpp
 * @brief Validates the generic mission value model without robot hardware.
 */

#include <cassert>
#include <chrono>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include <humanoid/core.hpp>

namespace {

[[nodiscard]] humanoid::core::Command MakeCommand(humanoid::core::CommandId id) {
  humanoid::core::Command command;
  command.id = id;
  command.timestamp = humanoid::core::CommandTimestamp{std::chrono::milliseconds{10}};
  command.type = humanoid::core::CommandType::Stop;
  command.timeout = std::chrono::milliseconds{250};
  command.metadata.emplace("source", "mission-model-test");
  return command;
}

[[nodiscard]] humanoid::mission::MissionStep MakeStep(humanoid::mission::MissionStepId id,
                                                      humanoid::core::Command command) {
  humanoid::mission::MissionStep step;
  step.id = id;
  step.name = "Stop motion";
  step.command = std::move(command);
  step.timeout = std::chrono::milliseconds{500};
  step.retry = 1U;
  step.metadata.emplace("phase", "safety");
  return step;
}

void TestMissionStatus() {
  using humanoid::mission::toString;

  assert(toString(humanoid::mission::MissionStatus::Pending) == "Pending");
  assert(toString(humanoid::mission::MissionStatus::Running) == "Running");
  assert(toString(humanoid::mission::MissionStatus::Paused) == "Paused");
  assert(toString(humanoid::mission::MissionStatus::Completed) == "Completed");
  assert(toString(humanoid::mission::MissionStatus::Failed) == "Failed");
  assert(toString(humanoid::mission::MissionStatus::Cancelled) == "Cancelled");

  assert(!humanoid::mission::isTerminal(humanoid::mission::MissionStatus::Pending));
  assert(!humanoid::mission::isTerminal(humanoid::mission::MissionStatus::Paused));
  assert(humanoid::mission::isTerminal(humanoid::mission::MissionStatus::Completed));
  assert(humanoid::mission::isTerminal(humanoid::mission::MissionStatus::Failed));
  assert(humanoid::mission::isTerminal(humanoid::mission::MissionStatus::Cancelled));
}

void TestMissionResult() {
  humanoid::mission::MissionResult result;
  assert(result.status == humanoid::mission::MissionStatus::Pending);
  assert(!result.isSuccess());
  assert(!result.isTerminal());

  result.status = humanoid::mission::MissionStatus::Completed;
  result.message = "mission completed";
  assert(result.isSuccess());
  assert(result.isTerminal());

  result.status = humanoid::mission::MissionStatus::Failed;
  assert(!result.isSuccess());
  assert(result.isTerminal());
}

void TestMissionStepDefaults() {
  const humanoid::mission::MissionStep step;

  assert(step.id == 0U);
  assert(step.name.empty());
  assert(step.command.id == 0U);
  assert(step.timeout == humanoid::mission::MissionStepTimeout::zero());
  assert(step.retry == 0U);
  assert(step.retryPolicy.maxAttempts == 1U);
  assert(step.retryPolicy.delayBetweenAttempts == humanoid::mission::RetryDelay::zero());
  assert(step.loopPolicy.iterations == 1U);
  assert(step.timeoutPolicy.timeout == humanoid::mission::TimeoutDuration::zero());
  assert(!step.wait.has_value());
  assert(!step.delay.has_value());
  assert(!step.skip);
  assert(!step.abort);
  assert(step.enabled);
  assert(step.metadata.empty());
  assert(!step.isValid());
  assert(!step.hasTimeout());
  assert(!step.hasEffectiveTimeout());
}

void TestPopulatedMissionStep() {
  const humanoid::mission::MissionStep step = MakeStep(7U, MakeCommand(42U));

  assert(step.isValid());
  assert(step.hasTimeout());
  assert(step.command.isValid());
  assert(step.command.type == humanoid::core::CommandType::Stop);
  assert(step.retry == 1U);
  assert(step.metadata.at("phase") == "safety");
}

void TestFlowControlStepValidity() {
  humanoid::mission::MissionStep wait_step;
  wait_step.id = 8U;
  wait_step.name = "Wait";
  wait_step.wait = humanoid::mission::WaitStep{std::chrono::milliseconds{5}};
  assert(wait_step.isValid());

  humanoid::mission::MissionStep delay_step;
  delay_step.id = 9U;
  delay_step.name = "Delay";
  delay_step.delay = humanoid::mission::DelayStep{std::chrono::milliseconds{5}};
  delay_step.loopPolicy = humanoid::mission::LoopPolicy{2U};
  assert(delay_step.isValid());
  assert(delay_step.loopPolicy.isEnabled());

  humanoid::mission::MissionStep skipped_step;
  skipped_step.id = 10U;
  skipped_step.name = "Skip";
  skipped_step.skip = true;
  assert(skipped_step.isValid());

  humanoid::mission::MissionStep abort_step;
  abort_step.id = 11U;
  abort_step.name = "Abort";
  abort_step.abort = true;
  abort_step.timeoutPolicy = humanoid::mission::TimeoutPolicy{std::chrono::milliseconds{10}};
  assert(abort_step.isValid());
  assert(abort_step.hasEffectiveTimeout());
}

void TestMissionDefaults() {
  const humanoid::mission::Mission mission;

  assert(mission.id == 0U);
  assert(mission.name.empty());
  assert(mission.description.empty());
  assert(mission.version.empty());
  assert(mission.author.empty());
  assert(mission.steps.empty());
  assert(mission.metadata.empty());
  assert(!mission.hasSteps());
  assert(mission.stepCount() == 0U);
  assert(!mission.isValid());
}

void TestPopulatedMission() {
  humanoid::mission::Mission mission;
  mission.id = 100U;
  mission.name = "Safe stop";
  mission.description = "Requests a generic stop command.";
  mission.version = "1.0.0";
  mission.author = "humanoid-core";
  mission.timestamp = humanoid::mission::MissionTimestamp{std::chrono::milliseconds{25}};
  mission.steps.push_back(MakeStep(1U, MakeCommand(1U)));
  mission.metadata.emplace("domain", "validation");

  assert(mission.hasSteps());
  assert(mission.stepCount() == 1U);
  assert(mission.isValid());
  assert(mission.metadata.at("domain") == "validation");
}

void TestDisabledInvalidStepDoesNotBlockMission() {
  humanoid::mission::Mission mission;
  mission.id = 101U;
  mission.steps.push_back(MakeStep(1U, MakeCommand(1U)));

  humanoid::mission::MissionStep disabled_step;
  disabled_step.enabled = false;
  mission.steps.push_back(disabled_step);

  assert(mission.stepCount() == 2U);
  assert(mission.isValid());
}

void TestMissionRejectsInvalidEnabledStep() {
  humanoid::mission::Mission mission;
  mission.id = 102U;
  mission.steps.push_back(MakeStep(1U, MakeCommand(1U)));

  humanoid::mission::MissionStep invalid_step;
  invalid_step.enabled = true;
  mission.steps.push_back(invalid_step);

  assert(!mission.isValid());
}

} // namespace

int main() {
  static_assert(
      std::is_same_v<std::underlying_type_t<humanoid::mission::MissionStatus>, std::uint8_t>);

  try {
    TestMissionStatus();
    TestMissionResult();
    TestMissionStepDefaults();
    TestPopulatedMissionStep();
    TestFlowControlStepValidity();
    TestMissionDefaults();
    TestPopulatedMission();
    TestDisabledInvalidStepDoesNotBlockMission();
    TestMissionRejectsInvalidEnabledStep();
  } catch (...) {
    return 1;
  }

  return 0;
}
