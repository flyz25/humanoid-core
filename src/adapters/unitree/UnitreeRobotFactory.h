#pragma once

/**
 * @file UnitreeRobotFactory.h
 * @brief Defines the Unitree robot adapter factory.
 */

#include <humanoid/adapters/IRobotFactory.h>

namespace humanoid::core {
class RobotStateManager;
} // namespace humanoid::core

namespace humanoid::adapters::unitree {

/**
 * @brief Factory for Unitree robot adapters.
 */
class UnitreeRobotFactory final : public IRobotFactory {
public:
  /**
   * @brief Constructs a factory without state-manager injection.
   */
  UnitreeRobotFactory() noexcept = default;

  /**
   * @brief Constructs a factory that injects a shared robot state manager into adapters.
   *
   * @param state_manager Optional state manager updated by read-only SDK communication.
   */
  explicit UnitreeRobotFactory(std::shared_ptr<core::RobotStateManager> state_manager) noexcept;

  /**
   * @brief Returns the Unitree vendor name.
   *
   * @return Vendor name.
   */
  [[nodiscard]] std::string_view Vendor() const noexcept override;

  /**
   * @brief Returns supported Unitree models.
   *
   * @return Supported model names.
   */
  [[nodiscard]] std::vector<std::string> SupportedModels() const override;

  /**
   * @brief Reports whether a vendor and model are supported.
   *
   * @param vendor Robot vendor name.
   * @param model Robot model name.
   * @return True when supported.
   */
  [[nodiscard]] bool Supports(std::string_view vendor, std::string_view model) const override;

  /**
   * @brief Creates a Unitree robot adapter.
   *
   * @param config Robot configuration.
   * @param logger Optional logger interface.
   * @return Adapter instance, or nullptr when unsupported.
   */
  [[nodiscard]] std::unique_ptr<IRobotAdapter>
  CreateAdapter(const RobotConfig& config, std::shared_ptr<logging::ILogger> logger) const override;

private:
  std::shared_ptr<core::RobotStateManager> state_manager_;
};

} // namespace humanoid::adapters::unitree
