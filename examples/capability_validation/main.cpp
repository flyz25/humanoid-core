/**
 * @file main.cpp
 * @brief Shows generic capability and safety validation without robot hardware.
 */

#include <chrono>
#include <cstdlib>
#include <iostream>

#include <humanoid/core/SafetyValidator.h>

namespace {

[[nodiscard]] humanoid::core::Command MakeMoveCommand(humanoid::core::CommandId id) {
  humanoid::core::Command command;
  command.id = id;
  command.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  command.type = humanoid::core::CommandType::Move;
  command.payload.emplace("linear_x", 0.1);
  command.payload.emplace("linear_y", 0.0);
  command.payload.emplace("angular_z", 0.0);
  return command;
}

[[nodiscard]] humanoid::core::SafetyValidationContext SafeContext() {
  humanoid::core::SafetyValidationContext context;
  context.capabilities = humanoid::core::CommandCapabilitySet::LegacyAdapterDefaults();
  context.robotState.connection.connected = true;
  context.robotState.power.batteryLevel = 80.0F;
  context.robotState.motion.standing = true;
  return context;
}

} // namespace

int main() {
  const humanoid::core::SafetyValidator validator;

  humanoid::core::SafetyValidationContext context = SafeContext();
  const humanoid::core::CommandResult accepted = validator.Validate(MakeMoveCommand(1U), context);
  if (!accepted.isSuccess()) {
    return EXIT_FAILURE;
  }

  context.capabilities.move = false;
  const humanoid::core::CommandResult rejected = validator.Validate(MakeMoveCommand(2U), context);
  if (rejected.status != humanoid::core::CommandStatus::Rejected) {
    return EXIT_FAILURE;
  }

  std::cout << "Capability validation rejected unsupported Move command\n";
  return EXIT_SUCCESS;
}
