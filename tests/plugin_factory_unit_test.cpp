#include <humanoid/common/Status.hpp>
#include <humanoid/common/Version.hpp>
#include <humanoid/plugins/IPlugin.hpp>
#include <humanoid/plugins/IPluginRegistrar.hpp>
#include <humanoid/plugins/PluginFactory.hpp>
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
#include <thread>
#include <utility>
#include <vector>

namespace {

using humanoid::common::Status;
using humanoid::common::StatusCode;
using humanoid::plugins::IPlugin;
using humanoid::plugins::IPluginRegistrar;
using humanoid::plugins::PluginCreationResult;
using humanoid::plugins::PluginFactory;
using humanoid::plugins::PluginLifecycleState;
using humanoid::plugins::PluginMetadata;
using humanoid::plugins::PluginRecord;
using humanoid::plugins::PluginRegistry;
using humanoid::plugins::PluginVersionCompatibility;

[[nodiscard]] bool Fail(std::string_view test_name, std::string_view message) {
  std::cerr << test_name << ": " << message << '\n';
  return false;
}

[[nodiscard]] PluginMetadata MakeMetadata(std::string plugin_id) {
  PluginMetadata metadata;
  metadata.plugin_id = std::move(plugin_id);
  metadata.name = "Factory Test Plugin";
  metadata.vendor = "Humanoid Core";
  metadata.description = "Plugin factory unit test";
  metadata.version = humanoid::common::apiVersion();
  metadata.compatibility =
      PluginVersionCompatibility{humanoid::common::apiVersion(), humanoid::common::apiVersion()};
  return metadata;
}

class FactoryTestPlugin final : public IPlugin {
public:
  explicit FactoryTestPlugin(PluginMetadata metadata) : metadata_(std::move(metadata)) {}

  [[nodiscard]] const PluginMetadata& Metadata() const noexcept override { return metadata_; }

  [[nodiscard]] Status Initialize(IPluginRegistrar& registrar) override {
    initialized_ = true;
    return registrar.SetLifecycleState(metadata_.plugin_id, PluginLifecycleState::kInitialized);
  }

  [[nodiscard]] Status Start() override {
    if (!initialized_) {
      return Status::error(StatusCode::kFailedPrecondition, "plugin is not initialized");
    }

    return Status::ok();
  }

  [[nodiscard]] Status Stop() override { return Status::ok(); }

  [[nodiscard]] Status Shutdown() override {
    initialized_ = false;
    return Status::ok();
  }

private:
  PluginMetadata metadata_;
  bool initialized_{false};
};

[[nodiscard]] PluginFactory::PluginCreator MakeCreator(PluginMetadata metadata) {
  return [metadata = std::move(metadata)]() -> std::unique_ptr<IPlugin> {
    return std::make_unique<FactoryTestPlugin>(metadata);
  };
}

[[nodiscard]] bool TestFactoryRegisterCreateDestroyEnumerate() {
  constexpr std::string_view kTestName{"PluginFactory register/create/destroy"};

  auto registry = std::make_shared<PluginRegistry>();
  PluginFactory factory{registry};
  PluginMetadata metadata = MakeMetadata("org.humanoid.factory.lifecycle");

  if (!factory.RegisterPlugin(metadata, MakeCreator(metadata)).isOk()) {
    return Fail(kTestName, "plugin registration failed");
  }

  if (factory.RegisteredPluginCount() != 1U) {
    return Fail(kTestName, "registered plugin count was not updated");
  }

  const std::vector<PluginRecord> registered_plugins = factory.EnumeratePlugins();
  if (registered_plugins.size() != 1U ||
      registered_plugins.front().metadata.plugin_id != metadata.plugin_id) {
    return Fail(kTestName, "enumeration did not return the registered plugin");
  }

  PluginCreationResult creation_result = factory.CreatePlugin(metadata.plugin_id);
  if (!creation_result.status.isOk() || !creation_result.plugin) {
    return Fail(kTestName, "plugin creation failed");
  }

  if (registry->LifecycleState(metadata.plugin_id).value() != PluginLifecycleState::kLoaded) {
    return Fail(kTestName, "plugin lifecycle state was not updated to loaded");
  }

  if (factory.UnregisterPlugin(metadata.plugin_id).isOk()) {
    return Fail(kTestName, "unregister succeeded while plugin was active");
  }

  if (!factory.DestroyPlugin(creation_result.plugin).isOk() || creation_result.plugin) {
    return Fail(kTestName, "plugin destruction failed");
  }

  if (registry->LifecycleState(metadata.plugin_id).value() != PluginLifecycleState::kShutdown) {
    return Fail(kTestName, "plugin lifecycle state was not updated to shutdown");
  }

  if (!factory.UnregisterPlugin(metadata.plugin_id).isOk() ||
      factory.RegisteredPluginCount() != 0U) {
    return Fail(kTestName, "unregister after destruction failed");
  }

  return true;
}

[[nodiscard]] bool TestFactoryRejectsInvalidOperations() {
  constexpr std::string_view kTestName{"PluginFactory invalid operations"};

  PluginFactory factory;
  PluginMetadata metadata = MakeMetadata("org.humanoid.factory.invalid");

  if (factory.RegisterPlugin(metadata, PluginFactory::PluginCreator{}).isOk()) {
    return Fail(kTestName, "empty creator was accepted");
  }

  if (factory.CreatePlugin("missing.plugin").status.isOk()) {
    return Fail(kTestName, "unknown plugin creation succeeded");
  }

  if (!factory.RegisterPlugin(metadata, MakeCreator(metadata)).isOk()) {
    return Fail(kTestName, "valid plugin registration failed");
  }

  if (factory.RegisterPlugin(metadata, MakeCreator(metadata)).isOk()) {
    return Fail(kTestName, "duplicate plugin registration succeeded");
  }

  std::unique_ptr<IPlugin> null_plugin;
  if (factory.DestroyPlugin(null_plugin).isOk()) {
    return Fail(kTestName, "null plugin destruction succeeded");
  }

  PluginMetadata null_metadata = MakeMetadata("org.humanoid.factory.null");
  if (!factory.RegisterPlugin(null_metadata, []() { return std::unique_ptr<IPlugin>{}; }).isOk()) {
    return Fail(kTestName, "null-creator plugin registration failed");
  }

  if (factory.CreatePlugin(null_metadata.plugin_id).status.isOk()) {
    return Fail(kTestName, "creator returning null produced a successful result");
  }

  return true;
}

[[nodiscard]] bool TestFactoryRejectsMismatchedCreatorMetadata() {
  constexpr std::string_view kTestName{"PluginFactory mismatched creator metadata"};

  PluginFactory factory;
  PluginMetadata metadata = MakeMetadata("org.humanoid.factory.expected");
  PluginMetadata other_metadata = MakeMetadata("org.humanoid.factory.actual");

  const Status registration_status = factory.RegisterPlugin(metadata, MakeCreator(other_metadata));
  if (!registration_status.isOk()) {
    return Fail(kTestName, "registration failed before mismatch validation");
  }

  const PluginCreationResult creation_result = factory.CreatePlugin(metadata.plugin_id);
  if (creation_result.status.isOk() || creation_result.plugin) {
    return Fail(kTestName, "mismatched creator metadata was accepted");
  }

  return true;
}

[[nodiscard]] bool TestFactoryConcurrentCreateDestroy() {
  constexpr std::string_view kTestName{"PluginFactory concurrent create/destroy"};
  constexpr std::size_t kThreadCount{8};
  constexpr std::size_t kIterations{50};

  PluginFactory factory;
  PluginMetadata metadata = MakeMetadata("org.humanoid.factory.concurrent");

  if (!factory.RegisterPlugin(metadata, MakeCreator(metadata)).isOk()) {
    return Fail(kTestName, "registration failed");
  }

  std::latch start_gate{1};
  std::atomic<std::size_t> success_count{0};
  std::vector<std::jthread> workers;
  workers.reserve(kThreadCount);

  for (std::size_t thread_index = 0; thread_index < kThreadCount; ++thread_index) {
    (void)thread_index;
    workers.emplace_back([&factory, &metadata, &start_gate, &success_count]() {
      start_gate.wait();

      for (std::size_t iteration = 0; iteration < kIterations; ++iteration) {
        (void)iteration;
        PluginCreationResult creation_result = factory.CreatePlugin(metadata.plugin_id);
        if (creation_result.status.isOk() && creation_result.plugin &&
            factory.DestroyPlugin(creation_result.plugin).isOk() && !creation_result.plugin) {
          success_count.fetch_add(1U, std::memory_order_relaxed);
        }
      }
    });
  }

  start_gate.count_down();
  workers.clear();

  if (success_count.load(std::memory_order_relaxed) != kThreadCount * kIterations) {
    return Fail(kTestName, "not all concurrent create/destroy operations succeeded");
  }

  if (!factory.UnregisterPlugin(metadata.plugin_id).isOk()) {
    return Fail(kTestName, "unregister after concurrent operations failed");
  }

  return true;
}

[[nodiscard]] bool TestFactoryRejectsUnregisterDuringCreation() {
  constexpr std::string_view kTestName{"PluginFactory unregister during creation"};

  PluginFactory factory;
  PluginMetadata metadata = MakeMetadata("org.humanoid.factory.inflight");

  std::latch creator_entered{1};
  std::latch release_creator{1};

  if (!factory
           .RegisterPlugin(metadata,
                           [metadata, &creator_entered,
                            &release_creator]() -> std::unique_ptr<IPlugin> {
                             creator_entered.count_down();
                             release_creator.wait();
                             return std::make_unique<FactoryTestPlugin>(metadata);
                           })
           .isOk()) {
    return Fail(kTestName, "registration failed");
  }

  PluginCreationResult creation_result;
  std::jthread worker{[&factory, &metadata, &creation_result]() {
    creation_result = factory.CreatePlugin(metadata.plugin_id);
  }};

  creator_entered.wait();
  const Status unregister_status = factory.UnregisterPlugin(metadata.plugin_id);
  release_creator.count_down();
  worker.join();

  if (unregister_status.isOk()) {
    return Fail(kTestName, "unregister succeeded while creation was in flight");
  }

  if (!creation_result.status.isOk() || !creation_result.plugin) {
    return Fail(kTestName, "creation did not finish after release");
  }

  if (!factory.DestroyPlugin(creation_result.plugin).isOk()) {
    return Fail(kTestName, "destroy after in-flight creation failed");
  }

  if (!factory.UnregisterPlugin(metadata.plugin_id).isOk()) {
    return Fail(kTestName, "unregister after destruction failed");
  }

  return true;
}

} // namespace

int main() {
  const std::vector<bool (*)()> tests{
      TestFactoryRegisterCreateDestroyEnumerate,   TestFactoryRejectsInvalidOperations,
      TestFactoryRejectsMismatchedCreatorMetadata, TestFactoryConcurrentCreateDestroy,
      TestFactoryRejectsUnregisterDuringCreation,
  };

  for (const auto test : tests) {
    if (!test()) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
