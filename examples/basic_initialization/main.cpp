#include <cstdlib>
#include <iostream>
#include <memory>

#include <humanoid/configuration/ConfigManager.hpp>
#include <humanoid/core/CoreContext.hpp>
#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/logging/LoggerManager.hpp>
#include <humanoid/services/TelemetryService.h>

int main() {
  const humanoid::logging::LoggerManager logger;
  const humanoid::configuration::ConfigManager config;
  humanoid::core::CoreContext context;
  auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
  context.setRobotStateManager(state_manager);
  const humanoid::services::TelemetryService telemetry{context.robotStateManager()};

  (void)logger;
  (void)config;
  (void)telemetry;

  std::cout << "Humanoid Core Initialized" << '\n';
  return EXIT_SUCCESS;
}
