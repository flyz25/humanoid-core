#pragma once

/**
 * @file IRobotFactory.h
 * @brief Defines the robot factory interface.
 */

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <humanoid/adapters/IRobotAdapter.h>

namespace humanoid::logging {
class ILogger;
} // namespace humanoid::logging

namespace humanoid::adapters {

/**
 * @brief Abstract factory for creating robot adapters.
 */
class IRobotFactory {
public:
  /**
   * @brief Destroys the factory interface.
   */
  virtual ~IRobotFactory() = default;

  /**
   * @brief Returns the vendor handled by this factory.
   *
   * @return Vendor name.
   */
  [[nodiscard]] virtual std::string_view Vendor() const noexcept = 0;

  /**
   * @brief Returns the robot models supported by this factory.
   *
   * @return Supported model names.
   */
  [[nodiscard]] virtual std::vector<std::string> SupportedModels() const = 0;

  /**
   * @brief Reports whether this factory supports a vendor and model pair.
   *
   * @param vendor Robot vendor name.
   * @param model Robot model name.
   * @return True when the factory can create an adapter for the pair.
   */
  [[nodiscard]] virtual bool Supports(std::string_view vendor, std::string_view model) const = 0;

  /**
   * @brief Creates a robot adapter.
   *
   * @param config Robot configuration.
   * @param logger Optional logger interface.
   * @return Adapter instance, or nullptr when unsupported.
   */
  [[nodiscard]] virtual std::unique_ptr<IRobotAdapter>
  CreateAdapter(const RobotConfig& config, std::shared_ptr<logging::ILogger> logger) const = 0;
};

} // namespace humanoid::adapters
