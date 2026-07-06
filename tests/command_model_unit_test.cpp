/**
 * @file command_model_unit_test.cpp
 * @brief Validates the generic command value model without robot hardware.
 */

#include <cassert>
#include <chrono>
#include <cstdint>
#include <string>
#include <type_traits>

#include <humanoid/core/Command.h>
#include <humanoid/core/CommandResult.h>

namespace {

void TestCommandDefaults() {
  const humanoid::core::Command command;

  assert(command.id == 0U);
  assert(command.type == humanoid::core::CommandType::Custom);
  assert(command.priority == humanoid::core::CommandPriority::Normal);
  assert(command.timeout == humanoid::core::CommandTimeout::zero());
  assert(command.payload.empty());
  assert(command.metadata.empty());
  assert(!command.isValid());
  assert(!command.hasTimeout());
}

void TestPopulatedCommand() {
  humanoid::core::Command command;
  command.id = 42U;
  command.timestamp = humanoid::core::CommandTimestamp{std::chrono::milliseconds{125}};
  command.type = humanoid::core::CommandType::Move;
  command.priority = humanoid::core::CommandPriority::High;
  command.timeout = std::chrono::milliseconds{500};
  command.payload.emplace("linear_x", 0.5);
  command.payload.emplace("relative", true);
  command.payload.emplace("sequence", std::int64_t{7});
  command.payload.emplace("frame", std::string{"base"});
  command.metadata.emplace("correlation_id", "motion-42");

  assert(command.isValid());
  assert(command.hasTimeout());
  assert(std::get<double>(command.payload.at("linear_x")) == 0.5);
  assert(std::get<bool>(command.payload.at("relative")));
  assert(std::get<std::int64_t>(command.payload.at("sequence")) == 7);
  assert(std::get<std::string>(command.payload.at("frame")) == "base");
  assert(command.metadata.at("correlation_id") == "motion-42");
}

void TestInvalidTimeout() {
  humanoid::core::Command command;
  command.id = 1U;
  command.timeout = std::chrono::milliseconds{-1};

  assert(!command.isValid());
  assert(!command.hasTimeout());
}

void TestCommandResult() {
  humanoid::core::CommandResult result;
  assert(result.status == humanoid::core::CommandStatus::Pending);
  assert(!result.isSuccess());
  assert(!result.isTerminal());

  result.status = humanoid::core::CommandStatus::Completed;
  result.message = "completed";
  assert(result.isSuccess());
  assert(result.isTerminal());

  result.status = humanoid::core::CommandStatus::Rejected;
  assert(!result.isSuccess());
  assert(result.isTerminal());
}

void TestStableEnumNames() {
  using humanoid::core::toString;

  assert(toString(humanoid::core::CommandType::Stand) == "Stand");
  assert(toString(humanoid::core::CommandType::Sit) == "Sit");
  assert(toString(humanoid::core::CommandType::Velocity) == "Velocity");
  assert(toString(humanoid::core::CommandType::EmergencyStop) == "EmergencyStop");
  assert(toString(humanoid::core::CommandType::Gesture) == "Gesture");
  assert(toString(humanoid::core::CommandType::PlayAudio) == "PlayAudio");
  assert(toString(humanoid::core::CommandType::SetVolume) == "SetVolume");
  assert(toString(humanoid::core::CommandType::MuteAudio) == "MuteAudio");
  assert(toString(humanoid::core::CommandStatus::Running) == "Running");
  assert(toString(humanoid::core::CommandStatus::Timeout) == "Timeout");
  assert(toString(humanoid::core::CommandPriority::Critical) == "Critical");
}

} // namespace

int main() {
  static_assert(std::is_same_v<std::underlying_type_t<humanoid::core::CommandType>, std::uint8_t>);
  static_assert(
      std::is_same_v<std::underlying_type_t<humanoid::core::CommandStatus>, std::uint8_t>);
  static_assert(
      std::is_same_v<std::underlying_type_t<humanoid::core::CommandPriority>, std::uint8_t>);

  try {
    TestCommandDefaults();
    TestPopulatedCommand();
    TestInvalidTimeout();
    TestCommandResult();
    TestStableEnumNames();
  } catch (...) {
    return 1;
  }

  return 0;
}
