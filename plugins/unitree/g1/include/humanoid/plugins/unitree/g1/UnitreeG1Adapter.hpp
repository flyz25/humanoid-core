#pragma once

/**
 * @file UnitreeG1Adapter.hpp
 * @brief Defines the Unitree G1 plugin adapter skeleton.
 */

#include <atomic>

#include <humanoid/common/Status.hpp>
#include <humanoid/core/RobotAdapter.h>

namespace humanoid::plugins::unitree::g1 {

/**
 * @brief Unitree G1 robot adapter skeleton exposed by the Unitree G1 plugin.
 *
 * The skeleton implements the vendor-independent `humanoid::core::RobotAdapter`
 * contract without linking Unitree SDK2 or communicating with physical robot
 * hardware. It returns conservative mock state until a future milestone adds a
 * real SDK-backed plugin implementation.
 */
class UnitreeG1Adapter final : public humanoid::core::RobotAdapter {
public:
  /**
   * @brief Constructs a disconnected Unitree G1 adapter skeleton.
   */
  UnitreeG1Adapter() = default;

  /**
   * @brief Destroys the adapter skeleton.
   */
  ~UnitreeG1Adapter() override = default;

  UnitreeG1Adapter(const UnitreeG1Adapter&) = delete;
  UnitreeG1Adapter& operator=(const UnitreeG1Adapter&) = delete;
  UnitreeG1Adapter(UnitreeG1Adapter&&) = delete;
  UnitreeG1Adapter& operator=(UnitreeG1Adapter&&) = delete;

  /**
   * @brief Initializes local skeleton state.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Initialize() override;

  /**
   * @brief Releases local skeleton state.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Shutdown() override;

  /**
   * @brief Reports that physical Unitree G1 communication is unavailable.
   *
   * @return Unavailable status because SDK communication is out of scope.
   */
  [[nodiscard]] humanoid::common::Status Connect() override;

  /**
   * @brief Clears local connection state.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Disconnect() override;

  /**
   * @brief Reports whether physical robot communication is established.
   *
   * @return Always false for the skeleton because no SDK communication exists.
   */
  [[nodiscard]] bool IsConnected() const noexcept override;

  /**
   * @brief Returns a conservative mock robot state snapshot.
   *
   * @return Vendor-independent robot state.
   */
  [[nodiscard]] humanoid::core::RobotState GetRobotState() const override;

  /**
   * @brief Returns static Unitree G1 plugin adapter information.
   *
   * @return Vendor-independent robot information.
   */
  [[nodiscard]] humanoid::core::RobotInformation GetRobotInformation() const override;

  /**
   * @brief Returns skeleton capability metadata.
   *
   * @return Vendor-independent capability declaration.
   */
  [[nodiscard]] humanoid::core::RobotCapabilities GetCapabilities() const override;

  /**
   * @brief Performs a local non-blocking skeleton update.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Update() override;

private:
  std::atomic_bool initialized_{false};
  std::atomic_bool connected_{false};
};

} // namespace humanoid::plugins::unitree::g1
