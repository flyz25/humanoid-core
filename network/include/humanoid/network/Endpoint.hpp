#pragma once

/**
 * @file Endpoint.hpp
 * @brief Defines network endpoint metadata.
 */

#include <cstdint>
#include <string>

#include <humanoid/network/TransportProtocol.hpp>

namespace humanoid::network {

/**
 * @brief Represents a network endpoint address.
 */
class Endpoint final {
public:
  /**
   * @brief Constructs an endpoint.
   *
   * @param host Hostname or address.
   * @param port Transport port.
   * @param protocol Transport protocol category.
   */
  Endpoint(std::string host, std::uint16_t port, TransportProtocol protocol);

  /**
   * @brief Returns the endpoint host.
   *
   * @return Hostname or address.
   */
  [[nodiscard]] const std::string& host() const noexcept;

  /**
   * @brief Returns the endpoint port.
   *
   * @return Transport port.
   */
  [[nodiscard]] std::uint16_t port() const noexcept;

  /**
   * @brief Returns the endpoint protocol.
   *
   * @return Transport protocol category.
   */
  [[nodiscard]] TransportProtocol protocol() const noexcept;

  /**
   * @brief Formats the endpoint for diagnostics.
   *
   * @return Stable endpoint string.
   */
  [[nodiscard]] std::string toString() const;

private:
  std::string host_;
  std::uint16_t port_;
  TransportProtocol protocol_;
};

} // namespace humanoid::network
