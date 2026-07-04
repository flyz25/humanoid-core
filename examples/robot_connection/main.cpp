#include "common/ExampleRobotConfig.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <factory/RobotFactoryRegistry.h>
#include <humanoid/adapters/IRobotAdapter.h>
#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/logging/LoggerManager.hpp>

#if HUMANOID_CORE_HAS_UNITREE
#include <adapters/unitree/UnitreeRobotFactory.h>
#endif

namespace {

/**
 * @brief Throws when an adapter result reports failure.
 *
 * @param result Result to inspect.
 * @param operation Operation name.
 */
void RequireSuccess(const humanoid::adapters::Result& result, const char* operation) {
  if (!result.Succeeded()) {
    throw std::runtime_error(std::string{operation} + " failed: " + result.message);
  }
}

} // namespace

int main(int argc, char* argv[]) {
  try {
    const humanoid::adapters::RobotConfig config = humanoid::examples::LoadRobotConfig(
        humanoid::examples::ConfigPathFromArguments(argc, argv, "config/robot.yaml"));

    auto logger = std::make_shared<humanoid::logging::LoggerManager>();
    auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
    humanoid::factory::RobotFactoryRegistry registry;

#if HUMANOID_CORE_HAS_UNITREE
    RequireSuccess(
        registry.RegisterFactory(
            std::make_shared<humanoid::adapters::unitree::UnitreeRobotFactory>(state_manager)),
        "RegisterFactory");
#endif

    std::unique_ptr<humanoid::adapters::IRobotAdapter> adapter =
        registry.CreateAdapter(config, logger);
    if (!adapter) {
      std::cout << "No robot adapter registered for " << config.vendor << ' ' << config.model
                << '\n';
      return EXIT_SUCCESS;
    }

    if (!humanoid::examples::HasArgumentFlag(argc, argv, "--execute")) {
      std::cout << "Robot connection example prepared " << config.vendor << ' ' << config.model
                << ". Pass --execute to initialize read-only physical communication." << '\n';
      return EXIT_SUCCESS;
    }

    const humanoid::adapters::Result initialize = adapter->Initialize();
    if (!initialize.Succeeded()) {
      std::cout << "Robot initialization unavailable: " << initialize.message << '\n';
      return EXIT_SUCCESS;
    }

    const humanoid::adapters::Result connect = adapter->Connect();
    if (!connect.Succeeded()) {
      static_cast<void>(adapter->Shutdown());
      std::cout << "Robot connection unavailable: " << connect.message << '\n';
      return EXIT_SUCCESS;
    }

    RequireSuccess(adapter->Disconnect(), "Disconnect");
    RequireSuccess(adapter->Shutdown(), "Shutdown");
    std::cout << "Robot connection example completed" << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
