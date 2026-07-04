#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <humanoid/common/Status.hpp>
#include <humanoid/core/RobotAdapter.h>
#include <humanoid/plugins/PluginFactory.hpp>
#include <humanoid/plugins/PluginRegistry.hpp>
#include <humanoid/plugins/unitree/g1/UnitreeG1Plugin.hpp>

namespace {

/**
 * @brief Throws when a status reports failure.
 *
 * @param status Status to inspect.
 * @param operation Operation name.
 */
void RequireOk(const humanoid::common::Status& status, const std::string& operation) {
  if (!status.isOk()) {
    throw std::runtime_error(operation + " failed: " + status.message());
  }
}

/**
 * @brief Converts a boolean capability to stable text.
 *
 * @param value Capability value.
 * @return "yes" when true, otherwise "no".
 */
[[nodiscard]] const char* YesNo(bool value) noexcept { return value ? "yes" : "no"; }

} // namespace

int main() {
  try {
    auto registry = std::make_shared<humanoid::plugins::PluginRegistry>();
    humanoid::plugins::PluginFactory factory{registry};
    RequireOk(humanoid::plugins::unitree::g1::RegisterUnitreeG1Plugin(factory),
              "RegisterUnitreeG1Plugin");

    humanoid::plugins::PluginCreationResult creation = factory.CreatePlugin("com.unitree.g1");
    RequireOk(creation.status, "CreatePlugin");
    auto* unitree_plugin =
        dynamic_cast<humanoid::plugins::unitree::g1::UnitreeG1Plugin*>(creation.plugin.get());
    if (unitree_plugin == nullptr) {
      throw std::runtime_error("created plugin does not expose Unitree G1 integration API");
    }

    std::unique_ptr<humanoid::core::RobotAdapter> adapter = unitree_plugin->CreateAdapter();
    RequireOk(adapter->Initialize(), "Adapter.Initialize");

    const humanoid::core::RobotInformation information = adapter->GetRobotInformation();
    const humanoid::core::RobotCapabilities capabilities = adapter->GetCapabilities();

    std::cout << information.vendor << ' ' << information.model << " capabilities" << '\n';
    std::cout << "state_feedback=" << YesNo(capabilities.supportsStateFeedback) << '\n';
    std::cout << "connection_management=" << YesNo(capabilities.supportsConnectionManagement)
              << '\n';
    std::cout << "periodic_update=" << YesNo(capabilities.supportsPeriodicUpdate) << '\n';
    std::cout << "health_state=" << YesNo(capabilities.supportsHealthState) << '\n';

    RequireOk(adapter->Shutdown(), "Adapter.Shutdown");
    RequireOk(factory.DestroyPlugin(creation.plugin), "DestroyPlugin");
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
