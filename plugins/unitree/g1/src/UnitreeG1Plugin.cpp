#include <humanoid/plugins/unitree/g1/UnitreeG1Plugin.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <humanoid/common/Version.hpp>
#include <humanoid/plugins/IPluginRegistrar.hpp>
#include <humanoid/plugins/PluginFactory.hpp>
#include <humanoid/plugins/PluginLifecycleState.hpp>
#include <humanoid/plugins/PluginVersionCompatibility.hpp>
#include <humanoid/plugins/unitree/g1/UnitreeG1Adapter.hpp>

namespace humanoid::plugins::unitree::g1 {
namespace {

[[nodiscard]] humanoid::common::Status FailedPrecondition(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kFailedPrecondition,
                                         std::move(message));
}

} // namespace

UnitreeG1Plugin::UnitreeG1Plugin() : metadata_(CreateMetadata()) {}

humanoid::plugins::PluginMetadata UnitreeG1Plugin::CreateMetadata() {
  humanoid::plugins::PluginMetadata metadata;
  metadata.plugin_id = "com.unitree.g1";
  metadata.name = "Unitree G1 Plugin";
  metadata.vendor = "Unitree";
  metadata.description = "Unitree G1 plugin skeleton without SDK communication";
  metadata.version = humanoid::common::apiVersion();
  metadata.compatibility = humanoid::plugins::PluginVersionCompatibility{
      humanoid::common::apiVersion(), humanoid::common::apiVersion()};
  metadata.manifest_path = "plugins/unitree/g1/plugin_manifest.json";
  return metadata;
}

const humanoid::plugins::PluginMetadata& UnitreeG1Plugin::Metadata() const noexcept {
  return metadata_;
}

humanoid::common::Status
UnitreeG1Plugin::Initialize(humanoid::plugins::IPluginRegistrar& registrar) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (initialized_) {
    return humanoid::common::Status::ok();
  }

  humanoid::common::Status lifecycle_status = registrar.SetLifecycleState(
      metadata_.plugin_id, humanoid::plugins::PluginLifecycleState::kInitialized);

  if (!lifecycle_status.isOk()) {
    const humanoid::common::Status registration_status = registrar.RegisterPlugin(metadata_);
    if (!registration_status.isOk()) {
      return registration_status;
    }

    lifecycle_status = registrar.SetLifecycleState(
        metadata_.plugin_id, humanoid::plugins::PluginLifecycleState::kInitialized);
  }

  if (!lifecycle_status.isOk()) {
    return lifecycle_status;
  }

  initialized_ = true;
  started_ = false;
  return humanoid::common::Status::ok();
}

humanoid::common::Status UnitreeG1Plugin::Start() {
  std::lock_guard<std::mutex> lock{mutex_};
  if (!initialized_) {
    return FailedPrecondition("Unitree G1 plugin skeleton is not initialized");
  }

  started_ = true;
  return humanoid::common::Status::ok();
}

humanoid::common::Status UnitreeG1Plugin::Stop() {
  std::lock_guard<std::mutex> lock{mutex_};
  started_ = false;
  return humanoid::common::Status::ok();
}

humanoid::common::Status UnitreeG1Plugin::Shutdown() {
  std::lock_guard<std::mutex> lock{mutex_};
  started_ = false;
  initialized_ = false;
  return humanoid::common::Status::ok();
}

std::unique_ptr<humanoid::core::RobotAdapter> UnitreeG1Plugin::CreateAdapter() const {
  return std::make_unique<UnitreeG1Adapter>();
}

humanoid::common::Status RegisterUnitreeG1Plugin(humanoid::plugins::PluginFactory& factory) {
  return factory.RegisterPlugin(UnitreeG1Plugin::CreateMetadata(),
                                []() -> std::unique_ptr<humanoid::plugins::IPlugin> {
                                  return std::make_unique<UnitreeG1Plugin>();
                                });
}

} // namespace humanoid::plugins::unitree::g1
