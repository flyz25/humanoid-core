#include <humanoid/common/Status.hpp>
#include <humanoid/common/Version.hpp>
#include <humanoid/plugins/IPlugin.hpp>
#include <humanoid/plugins/PluginLifecycleState.hpp>
#include <humanoid/plugins/PluginMetadata.hpp>
#include <humanoid/plugins/PluginRegistry.hpp>
#include <humanoid/plugins/PluginVersionCompatibility.hpp>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <latch>
#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

namespace {

using humanoid::common::SemanticVersion;
using humanoid::common::Status;
using humanoid::common::StatusCode;
using humanoid::plugins::IPlugin;
using humanoid::plugins::IPluginRegistrar;
using humanoid::plugins::PluginLifecycleState;
using humanoid::plugins::PluginMetadata;
using humanoid::plugins::PluginRegistry;
using humanoid::plugins::PluginVersionCompatibility;

[[nodiscard]] bool Fail(std::string_view test_name, std::string_view message) {
  std::cerr << test_name << ": " << message << '\n';
  return false;
}

[[nodiscard]] PluginMetadata MakeMetadata(std::string plugin_id) {
  PluginMetadata metadata;
  metadata.plugin_id = std::move(plugin_id);
  metadata.name = "Test Plugin";
  metadata.vendor = "Humanoid Core";
  metadata.description = "Plugin infrastructure unit test";
  metadata.version = SemanticVersion{0, 1, 0};
  metadata.compatibility = PluginVersionCompatibility{
      humanoid::common::apiVersion(), humanoid::common::apiVersion()};
  return metadata;
}

class TestPlugin final : public IPlugin {
public:
  explicit TestPlugin(PluginMetadata metadata) : metadata_(std::move(metadata)) {}

  [[nodiscard]] const PluginMetadata& Metadata() const noexcept override { return metadata_; }

  [[nodiscard]] Status Initialize(IPluginRegistrar& registrar) override {
    const Status registration_status = registrar.RegisterPlugin(metadata_);
    if (!registration_status.isOk()) {
      return registration_status;
    }

    initialized_ = true;
    return registrar.SetLifecycleState(metadata_.plugin_id,
                                       PluginLifecycleState::kInitialized);
  }

  [[nodiscard]] Status Start() override {
    if (!initialized_) {
      return Status::error(StatusCode::kFailedPrecondition,
                           "plugin must be initialized before start");
    }

    started_ = true;
    return Status::ok();
  }

  [[nodiscard]] Status Stop() override {
    if (!started_) {
      return Status::error(StatusCode::kFailedPrecondition,
                           "plugin must be started before stop");
    }

    started_ = false;
    return Status::ok();
  }

  [[nodiscard]] Status Shutdown() override {
    initialized_ = false;
    started_ = false;
    return Status::ok();
  }

private:
  PluginMetadata metadata_;
  bool initialized_{false};
  bool started_{false};
};

[[nodiscard]] bool TestPluginMetadataValidation() {
  constexpr std::string_view kTestName{"Plugin metadata validation"};

  PluginMetadata metadata = MakeMetadata("org.humanoid.test");
  if (!metadata.IsValid() ||
      !metadata.IsCompatibleWith(humanoid::common::apiVersion())) {
    return Fail(kTestName, "valid metadata was rejected");
  }

  PluginMetadata missing_id = MakeMetadata("");
  if (missing_id.IsValid()) {
    return Fail(kTestName, "metadata with empty plugin id was accepted");
  }

  const PluginVersionCompatibility invalid_range{SemanticVersion{0, 2, 0},
                                                SemanticVersion{0, 1, 0}};
  metadata.compatibility = invalid_range;
  if (metadata.IsValid()) {
    return Fail(kTestName, "metadata with invalid compatibility range was accepted");
  }

  return true;
}

[[nodiscard]] bool TestPluginRegistryRegistration() {
  constexpr std::string_view kTestName{"Plugin registry registration"};

  PluginRegistry registry;
  PluginMetadata metadata = MakeMetadata("org.humanoid.registry");

  if (!registry.RegisterPlugin(metadata).isOk()) {
    return Fail(kTestName, "registration failed");
  }

  if (registry.RegisterPlugin(metadata).isOk()) {
    return Fail(kTestName, "duplicate registration was accepted");
  }

  if (!registry.Contains(metadata.plugin_id) || registry.PluginCount() != 1U) {
    return Fail(kTestName, "registered plugin was not discoverable");
  }

  const std::optional<PluginMetadata> registered_metadata =
      registry.Metadata(metadata.plugin_id);
  if (!registered_metadata.has_value() ||
      registered_metadata->plugin_id != metadata.plugin_id) {
    return Fail(kTestName, "registered metadata lookup failed");
  }

  const std::optional<PluginLifecycleState> lifecycle_state =
      registry.LifecycleState(metadata.plugin_id);
  if (!lifecycle_state.has_value() ||
      lifecycle_state.value() != PluginLifecycleState::kRegistered) {
    return Fail(kTestName, "default lifecycle state was not registered");
  }

  if (!registry.SetLifecycleState(metadata.plugin_id, PluginLifecycleState::kLoaded)
           .isOk()) {
    return Fail(kTestName, "lifecycle update failed");
  }

  const std::optional<PluginLifecycleState> loaded_state =
      registry.LifecycleState(metadata.plugin_id);
  if (!loaded_state.has_value() ||
      loaded_state.value() != PluginLifecycleState::kLoaded) {
    return Fail(kTestName, "updated lifecycle state was not stored");
  }

  if (!registry.UnregisterPlugin(metadata.plugin_id).isOk() ||
      registry.Contains(metadata.plugin_id)) {
    return Fail(kTestName, "unregistration failed");
  }

  return true;
}

[[nodiscard]] bool TestPluginLifecycleThroughRegistrar() {
  constexpr std::string_view kTestName{"Plugin lifecycle through registrar"};

  PluginRegistry registry;
  TestPlugin plugin{MakeMetadata("org.humanoid.lifecycle")};

  if (!plugin.Initialize(registry).isOk()) {
    return Fail(kTestName, "plugin initialization failed");
  }

  const std::optional<PluginLifecycleState> initialized_state =
      registry.LifecycleState(plugin.Metadata().plugin_id);
  if (!initialized_state.has_value() ||
      initialized_state.value() != PluginLifecycleState::kInitialized) {
    return Fail(kTestName, "plugin did not publish initialized state");
  }

  if (!plugin.Start().isOk() || !plugin.Stop().isOk() ||
      !plugin.Shutdown().isOk()) {
    return Fail(kTestName, "plugin lifecycle operation failed");
  }

  return true;
}

[[nodiscard]] bool TestPluginRegistryThreadSafety() {
  constexpr std::string_view kTestName{"Plugin registry thread safety"};
  constexpr std::size_t kPluginCount{16};

  PluginRegistry registry;
  std::latch start_gate{1};
  std::atomic<std::size_t> success_count{0};
  std::vector<std::jthread> workers;
  workers.reserve(kPluginCount);

  for (std::size_t index = 0; index < kPluginCount; ++index) {
    workers.emplace_back([&registry, &start_gate, &success_count, index]() {
      start_gate.wait();

      PluginMetadata metadata =
          MakeMetadata("org.humanoid.concurrent." + std::to_string(index));
      if (registry.RegisterPlugin(std::move(metadata)).isOk()) {
        success_count.fetch_add(1U, std::memory_order_relaxed);
      }
    });
  }

  start_gate.count_down();
  workers.clear();

  if (success_count.load(std::memory_order_relaxed) != kPluginCount ||
      registry.PluginCount() != kPluginCount) {
    return Fail(kTestName, "not all concurrent registrations completed");
  }

  const std::vector<humanoid::plugins::PluginRecord> plugins = registry.Plugins();
  if (plugins.size() != kPluginCount) {
    return Fail(kTestName, "registry snapshot returned unexpected plugin count");
  }

  return true;
}

[[nodiscard]] bool TestPluginLifecycleStateStrings() {
  constexpr std::string_view kTestName{"Plugin lifecycle state strings"};

  if (humanoid::plugins::toString(PluginLifecycleState::kDiscovered) !=
          "discovered" ||
      humanoid::plugins::toString(PluginLifecycleState::kRegistered) !=
          "registered" ||
      humanoid::plugins::toString(PluginLifecycleState::kFailed) != "failed") {
    return Fail(kTestName, "lifecycle state string conversion failed");
  }

  return true;
}

} // namespace

int main() {
  const std::vector<bool (*)()> tests{
      TestPluginMetadataValidation,
      TestPluginRegistryRegistration,
      TestPluginLifecycleThroughRegistrar,
      TestPluginRegistryThreadSafety,
      TestPluginLifecycleStateStrings,
  };

  for (const auto test : tests) {
    if (!test()) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
