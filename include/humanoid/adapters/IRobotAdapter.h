#pragma once

/**
 * @file IRobotAdapter.h
 * @brief Compatibility include for the unified robot adapter contract.
 *
 * The repository previously exposed a second adapter abstraction from the
 * `humanoid::adapters` namespace. The single public robot adapter interface is
 * now `humanoid::core::RobotAdapter`. This header preserves existing include
 * paths and robot configuration types without defining another virtual
 * interface.
 */

#include <chrono>
#include <cstdint>
#include <string>

#include <humanoid/core/RobotAdapter.h>

namespace humanoid::adapters {

/**
 * @brief Robot configuration consumed by factories and adapters.
 */
struct RobotConfig final {
  /**
   * @brief Robot vendor name, for example "Unitree".
   */
  std::string vendor;

  /**
   * @brief Robot model name, for example "G1".
   */
  std::string model;

  /**
   * @brief Robot control IP address.
   */
  std::string ip{"192.168.123.161"};

  /**
   * @brief Network interface used by the vendor SDK transport.
   */
  std::string network_interface{"eth0"};

  /**
   * @brief Vendor SDK transport domain identifier.
   */
  std::uint32_t domain_id{0};

  /**
   * @brief Command timeout.
   */
  std::chrono::milliseconds timeout{500};

  /**
   * @brief Optional robot serial number.
   */
  std::string serial_number;

  /**
   * @brief Optional robot firmware version.
   */
  std::string firmware;
};

/**
 * @brief Compatibility alias for the single public adapter interface.
 */
using IRobotAdapter = humanoid::core::RobotAdapter;

} // namespace humanoid::adapters
