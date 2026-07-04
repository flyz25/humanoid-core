#pragma once

/**
 * @file UnitreeG1Plugin.hpp
 * @brief Defines the Unitree G1 plugin skeleton.
 */

#include <memory>
#include <mutex>

#include <humanoid/common/Status.hpp>
#include <humanoid/core/RobotAdapter.h>
#include <humanoid/plugins/IPlugin.hpp>
#include <humanoid/plugins/PluginMetadata.hpp>

namespace humanoid::plugins {
class PluginFactory;
} // namespace humanoid::plugins

namespace humanoid::plugins::unitree::g1 {

/**
 * @brief Unitree G1 plugin skeleton.
 *
 * The plugin owns lifecycle state and exposes a factory method for creating the
 * Unitree G1 adapter skeleton. It contains no Unitree SDK2 headers and performs
 * no robot communication.
 */
class UnitreeG1Plugin final : public humanoid::plugins::IPlugin {
public:
  /**
   * @brief Constructs the Unitree G1 plugin skeleton.
   */
  UnitreeG1Plugin();

  /**
   * @brief Destroys the plugin skeleton.
   */
  ~UnitreeG1Plugin() override = default;

  UnitreeG1Plugin(const UnitreeG1Plugin&) = delete;
  UnitreeG1Plugin& operator=(const UnitreeG1Plugin&) = delete;
  UnitreeG1Plugin(UnitreeG1Plugin&&) = delete;
  UnitreeG1Plugin& operator=(UnitreeG1Plugin&&) = delete;

  /**
   * @brief Creates metadata for the Unitree G1 plugin skeleton.
   *
   * @return Plugin metadata.
   */
  [[nodiscard]] static humanoid::plugins::PluginMetadata CreateMetadata();

  /**
   * @brief Returns immutable plugin metadata.
   *
   * @return Plugin metadata owned by this plugin instance.
   */
  [[nodiscard]] const humanoid::plugins::PluginMetadata& Metadata() const noexcept override;

  /**
   * @brief Initializes the plugin lifecycle through the supplied registrar.
   *
   * @param registrar Host registration boundary.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status
  Initialize(humanoid::plugins::IPluginRegistrar& registrar) override;

  /**
   * @brief Starts the initialized plugin skeleton.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Start() override;

  /**
   * @brief Stops the plugin skeleton.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Stop() override;

  /**
   * @brief Shuts down the plugin skeleton.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Shutdown() override;

  /**
   * @brief Creates a Unitree G1 adapter skeleton instance.
   *
   * @return Adapter owned through the generic robot adapter interface.
   */
  [[nodiscard]] std::unique_ptr<humanoid::core::RobotAdapter> CreateAdapter() const;

private:
  mutable std::mutex mutex_;
  humanoid::plugins::PluginMetadata metadata_;
  bool initialized_{false};
  bool started_{false};
};

/**
 * @brief Registers the Unitree G1 plugin skeleton with a plugin factory.
 *
 * @param factory Plugin factory owned by the host composition root.
 * @return Operation status.
 */
[[nodiscard]] humanoid::common::Status
RegisterUnitreeG1Plugin(humanoid::plugins::PluginFactory& factory);

} // namespace humanoid::plugins::unitree::g1
