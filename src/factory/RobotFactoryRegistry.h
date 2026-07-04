#pragma once

/**
 * @file RobotFactoryRegistry.h
 * @brief Defines a registry for robot adapter factories.
 */

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include <humanoid/adapters/IRobotFactory.h>

namespace humanoid::factory {

/**
 * @brief Thread-safe registry for robot adapter factories.
 */
class RobotFactoryRegistry final {
public:
  /**
   * @brief Constructs an empty factory registry.
   */
  RobotFactoryRegistry() = default;

  /**
   * @brief Copy construction is disabled because registry ownership is explicit.
   */
  RobotFactoryRegistry(const RobotFactoryRegistry&) = delete;

  /**
   * @brief Copy assignment is disabled because registry ownership is explicit.
   *
   * @return This registry.
   */
  RobotFactoryRegistry& operator=(const RobotFactoryRegistry&) = delete;

  /**
   * @brief Move construction is disabled because registered factories are protected by a mutex.
   */
  RobotFactoryRegistry(RobotFactoryRegistry&&) = delete;

  /**
   * @brief Move assignment is disabled because registered factories are protected by a mutex.
   *
   * @return This registry.
   */
  RobotFactoryRegistry& operator=(RobotFactoryRegistry&&) = delete;

  /**
   * @brief Registers a factory.
   *
   * @param factory Factory to register.
   * @return Registration result.
   */
  adapters::Result RegisterFactory(std::shared_ptr<adapters::IRobotFactory> factory);

  /**
   * @brief Finds a factory for a vendor and model.
   *
   * @param vendor Robot vendor name.
   * @param model Robot model name.
   * @return Matching factory, or nullptr when none is registered.
   */
  [[nodiscard]] std::shared_ptr<adapters::IRobotFactory> FindFactory(std::string_view vendor,
                                                                     std::string_view model) const;

  /**
   * @brief Creates an adapter through a matching registered factory.
   *
   * @param config Robot configuration.
   * @param logger Optional logger interface.
   * @return Adapter instance, or nullptr when no factory supports the config.
   */
  [[nodiscard]] std::unique_ptr<adapters::IRobotAdapter>
  CreateAdapter(const adapters::RobotConfig& config,
                std::shared_ptr<logging::ILogger> logger) const;

  /**
   * @brief Returns the number of registered factories.
   *
   * @return Factory count.
   */
  [[nodiscard]] std::size_t FactoryCount() const;

  /**
   * @brief Returns registered vendor names.
   *
   * @return Vendor names.
   */
  [[nodiscard]] std::vector<std::string> RegisteredVendors() const;

private:
  mutable std::mutex mutex_;
  std::vector<std::shared_ptr<adapters::IRobotFactory>> factories_;
};

} // namespace humanoid::factory
