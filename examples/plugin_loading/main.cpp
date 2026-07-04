#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <humanoid/common/Status.hpp>
#include <humanoid/plugins/PluginFactory.hpp>
#include <humanoid/plugins/PluginLifecycleState.hpp>
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

} // namespace

int main() {
  try {
    auto registry = std::make_shared<humanoid::plugins::PluginRegistry>();
    humanoid::plugins::PluginFactory factory{registry};

    RequireOk(humanoid::plugins::unitree::g1::RegisterUnitreeG1Plugin(factory),
              "RegisterUnitreeG1Plugin");

    const auto records = factory.EnumeratePlugins();
    for (const humanoid::plugins::PluginRecord& record : records) {
      std::cout << record.metadata.plugin_id << " [" << record.metadata.vendor << "] "
                << humanoid::plugins::toString(record.lifecycle_state) << '\n';
    }

    humanoid::plugins::PluginCreationResult creation = factory.CreatePlugin("com.unitree.g1");
    RequireOk(creation.status, "CreatePlugin");
    RequireOk(creation.plugin->Initialize(*registry), "Plugin.Initialize");
    RequireOk(creation.plugin->Start(), "Plugin.Start");
    RequireOk(creation.plugin->Stop(), "Plugin.Stop");
    RequireOk(factory.DestroyPlugin(creation.plugin), "DestroyPlugin");

    std::cout << "Plugin loading example completed with " << records.size()
              << " registered plugin(s)" << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
