#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <humanoid/common/Status.hpp>
#include <humanoid/core/RobotAdapter.h>
#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/plugins/PluginFactory.hpp>
#include <humanoid/plugins/PluginRegistry.hpp>
#include <humanoid/plugins/unitree/g1/UnitreeG1Plugin.hpp>
#include <humanoid/services/TelemetryService.h>

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

} // namespace

int main() {
  try {
    auto registry = std::make_shared<humanoid::plugins::PluginRegistry>();
    humanoid::plugins::PluginFactory plugin_factory{registry};
    RequireOk(humanoid::plugins::unitree::g1::RegisterUnitreeG1Plugin(plugin_factory),
              "RegisterUnitreeG1Plugin");

    humanoid::plugins::PluginCreationResult plugin_result =
        plugin_factory.CreatePlugin("com.unitree.g1");
    RequireOk(plugin_result.status, "CreatePlugin");
    RequireOk(plugin_result.plugin->Initialize(*registry), "Plugin.Initialize");
    RequireOk(plugin_result.plugin->Start(), "Plugin.Start");

    auto* unitree_plugin =
        dynamic_cast<humanoid::plugins::unitree::g1::UnitreeG1Plugin*>(plugin_result.plugin.get());
    if (unitree_plugin == nullptr) {
      throw std::runtime_error("created plugin does not expose Unitree G1 integration API");
    }

    std::unique_ptr<humanoid::core::RobotAdapter> adapter = unitree_plugin->CreateAdapter();
    RequireOk(adapter->Initialize(), "Adapter.Initialize");
    RequireOk(adapter->Update(), "Adapter.Update");

    auto state_manager = std::make_shared<humanoid::core::RobotStateManager>();
    state_manager->UpdateState(adapter->GetRobotState());

    std::atomic<int> telemetry_samples{0};
    humanoid::services::TelemetryService telemetry{state_manager, std::chrono::milliseconds{10}};
    const humanoid::services::TelemetryService::SubscriptionId subscription =
        telemetry.Subscribe([&telemetry_samples](const humanoid::core::RobotState& state) {
          if (!state.connection.connected) {
            telemetry_samples.fetch_add(1, std::memory_order_relaxed);
          }
        });
    if (subscription == humanoid::services::TelemetryService::kInvalidSubscriptionId) {
      throw std::runtime_error("failed to subscribe telemetry listener");
    }

    if (!telemetry.Start()) {
      throw std::runtime_error("failed to start telemetry service");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{30});
    telemetry.Stop();
    static_cast<void>(telemetry.Unsubscribe(subscription));

    const humanoid::core::RobotInformation information = adapter->GetRobotInformation();
    const humanoid::core::RobotCapabilities capabilities = adapter->GetCapabilities();

    RequireOk(adapter->Shutdown(), "Adapter.Shutdown");
    RequireOk(plugin_result.plugin->Stop(), "Plugin.Stop");
    RequireOk(plugin_factory.DestroyPlugin(plugin_result.plugin), "DestroyPlugin");

    std::cout << "Integrated " << information.vendor << ' ' << information.model
              << " plugin; state_feedback=" << (capabilities.supportsStateFeedback ? "yes" : "no")
              << "; telemetry_samples=" << telemetry_samples.load(std::memory_order_relaxed)
              << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
