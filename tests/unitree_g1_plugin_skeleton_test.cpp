#include <humanoid/common/Status.hpp>
#include <humanoid/core/RobotAdapter.h>
#include <humanoid/core/RobotState.hpp>
#include <humanoid/plugins/PluginFactory.hpp>
#include <humanoid/plugins/PluginLifecycleState.hpp>
#include <humanoid/plugins/unitree/g1/UnitreeG1Adapter.hpp>
#include <humanoid/plugins/unitree/g1/UnitreeG1Plugin.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace {

using humanoid::common::Status;
using humanoid::common::StatusCode;
using humanoid::core::RobotAdapter;
using humanoid::plugins::IPlugin;
using humanoid::plugins::PluginCreationResult;
using humanoid::plugins::PluginFactory;
using humanoid::plugins::PluginLifecycleState;
using humanoid::plugins::PluginMetadata;
using humanoid::plugins::unitree::g1::RegisterUnitreeG1Plugin;
using humanoid::plugins::unitree::g1::UnitreeG1Adapter;
using humanoid::plugins::unitree::g1::UnitreeG1Plugin;

[[nodiscard]] bool Fail(std::string_view test_name, std::string_view message) {
  std::cerr << test_name << ": " << message << '\n';
  return false;
}

[[nodiscard]] bool TestMetadataAndManifestContract() {
  constexpr std::string_view kTestName{"Unitree G1 plugin metadata"};

  const PluginMetadata metadata = UnitreeG1Plugin::CreateMetadata();
  if (!metadata.IsValid()) {
    return Fail(kTestName, "metadata is invalid");
  }

  if (metadata.plugin_id != "com.unitree.g1" || metadata.vendor != "Unitree" ||
      metadata.name != "Unitree G1 Plugin") {
    return Fail(kTestName, "metadata identity fields are incorrect");
  }

  if (metadata.manifest_path != "plugins/unitree/g1/plugin_manifest.json") {
    return Fail(kTestName, "manifest path is incorrect");
  }

  return true;
}

[[nodiscard]] bool TestFactoryRegistrationAndPluginLifecycle() {
  constexpr std::string_view kTestName{"Unitree G1 plugin lifecycle"};

  PluginFactory factory;
  const Status registration_status = RegisterUnitreeG1Plugin(factory);
  if (!registration_status.isOk()) {
    return Fail(kTestName, "factory registration failed");
  }

  const PluginMetadata metadata = UnitreeG1Plugin::CreateMetadata();
  PluginCreationResult creation_result = factory.CreatePlugin(metadata.plugin_id);
  if (!creation_result.status.isOk() || !creation_result.plugin) {
    return Fail(kTestName, "plugin creation failed");
  }

  IPlugin& plugin_interface = *creation_result.plugin;
  if (!plugin_interface.Initialize(*factory.Registry()).isOk()) {
    return Fail(kTestName, "plugin initialization failed");
  }

  if (factory.Registry()->LifecycleState(metadata.plugin_id).value() !=
      PluginLifecycleState::kInitialized) {
    return Fail(kTestName, "plugin lifecycle was not marked initialized");
  }

  if (!plugin_interface.Start().isOk() || !plugin_interface.Stop().isOk()) {
    return Fail(kTestName, "plugin start or stop failed");
  }

  const auto* unitree_plugin = dynamic_cast<UnitreeG1Plugin*>(&plugin_interface);
  if (unitree_plugin == nullptr) {
    return Fail(kTestName, "created plugin type is not UnitreeG1Plugin");
  }

  std::unique_ptr<RobotAdapter> adapter = unitree_plugin->CreateAdapter();
  if (!adapter) {
    return Fail(kTestName, "plugin did not create an adapter");
  }

  if (!factory.DestroyPlugin(creation_result.plugin).isOk() || creation_result.plugin) {
    return Fail(kTestName, "plugin destruction failed");
  }

  if (!factory.UnregisterPlugin(metadata.plugin_id).isOk()) {
    return Fail(kTestName, "factory unregister failed");
  }

  return true;
}

[[nodiscard]] bool TestAdapterProductionContract() {
  constexpr std::string_view kTestName{"Unitree G1 adapter production contract"};

  UnitreeG1Adapter adapter;

  if (adapter.IsConnected()) {
    return Fail(kTestName, "fresh adapter reported connected");
  }

  if (adapter.Connect().code() != StatusCode::kFailedPrecondition) {
    return Fail(kTestName, "connect before initialize did not fail precondition");
  }

  const Status initialize_status = adapter.Initialize();
  if (!initialize_status.isOk() && initialize_status.code() != StatusCode::kUnavailable) {
    return Fail(kTestName, "initialize failed with an unexpected status");
  }

  if (initialize_status.isOk()) {
    const Status update_status = adapter.Update();
    if (update_status.code() != StatusCode::kOk &&
        update_status.code() != StatusCode::kUnavailable) {
      return Fail(kTestName, "update failed with an unexpected status");
    }
  }

  const Status connect_status = adapter.Connect();
  if (connect_status.code() != StatusCode::kOk && connect_status.code() != StatusCode::kUnavailable &&
      connect_status.code() != StatusCode::kFailedPrecondition) {
    return Fail(kTestName, "connect failed with an unexpected status");
  }

  const humanoid::core::RobotInformation information = adapter.GetRobotInformation();
  if (information.vendor != "Unitree" || information.model != "G1") {
    return Fail(kTestName, "robot information is incorrect");
  }

  const humanoid::core::RobotCapabilities capabilities = adapter.GetCapabilities();
  if (!capabilities.supportsLifecycle || !capabilities.supportsConnectionManagement ||
      !capabilities.supportsStateFeedback || !capabilities.supportsRobotInformation ||
      !capabilities.supportsCommandExecution) {
    return Fail(kTestName, "capability declaration is incorrect");
  }

  const humanoid::core::CommandCapabilitySet command_capabilities =
      adapter.GetCommandCapabilities();
  if (!command_capabilities.stand || !command_capabilities.stop ||
      !command_capabilities.emergencyStop || !command_capabilities.move ||
      !command_capabilities.velocity) {
    return Fail(kTestName, "motion command capabilities are incomplete");
  }

  const humanoid::core::RobotState state = adapter.GetRobotState();
  if (state.connection.connected != adapter.IsConnected() || state.health.emergencyStop ||
      state.health.faultCode != 0) {
    return Fail(kTestName, "robot state is not conservative");
  }

  if (!adapter.Disconnect().isOk() || !adapter.Shutdown().isOk()) {
    return Fail(kTestName, "disconnect or shutdown failed");
  }

  return true;
}

} // namespace

int main() {
  const std::vector<bool (*)()> tests{
      TestMetadataAndManifestContract,
      TestFactoryRegistrationAndPluginLifecycle,
      TestAdapterProductionContract,
  };

  for (const auto test : tests) {
    if (!test()) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
