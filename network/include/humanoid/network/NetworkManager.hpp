#pragma once

/**
 * @file NetworkManager.hpp
 * @brief Defines the abstract network manager interface.
 */

#include <optional>

#include <humanoid/common/Status.hpp>
#include <humanoid/network/Endpoint.hpp>

namespace humanoid::network {

/**
 * @brief Abstract boundary for network lifecycle management.
 */
class NetworkManager {
public:
  /**
   * @brief Destroys the network manager interface.
   */
  virtual ~NetworkManager() = default;

  /**
   * @brief Opens a network endpoint.
   *
   * @param endpoint Endpoint to open.
   * @return Operation status.
   */
  virtual common::Status open(const Endpoint& endpoint) = 0;

  /**
   * @brief Closes the active network endpoint.
   *
   * @return Operation status.
   */
  virtual common::Status close() = 0;

  /**
   * @brief Reports whether a network endpoint is open.
   *
   * @return True when an endpoint is open.
   */
  [[nodiscard]] virtual bool isOpen() const noexcept = 0;

  /**
   * @brief Returns the active endpoint.
   *
   * @return Endpoint when one is open or configured.
   */
  [[nodiscard]] virtual std::optional<Endpoint> endpoint() const = 0;
};

} // namespace humanoid::network
