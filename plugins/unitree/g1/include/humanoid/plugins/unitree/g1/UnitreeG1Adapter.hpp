#pragma once

/**
 * @file UnitreeG1Adapter.hpp
 * @brief Defines the Unitree G1 plugin adapter.
 */

#include <memory>

#include <humanoid/common/Status.hpp>
#include <humanoid/core/RobotAdapter.h>
#include <humanoid/core/RobotStateManager.hpp>

#include <SdkTypes.h>

namespace humanoid::plugins::unitree::g1 {

/**
 * @brief Unitree G1 robot adapter exposed by the Unitree G1 plugin.
 *
 * The adapter implements the unified `humanoid::core::RobotAdapter` contract
 * and delegates all Unitree communication to the SDK abstraction layer.
 */
class UnitreeG1Adapter final : public humanoid::core::RobotAdapter {
public:
  /**
   * @brief Constructs a disconnected Unitree G1 adapter.
   */
  UnitreeG1Adapter();

  /**
   * @brief Constructs a Unitree G1 adapter with explicit SDK configuration.
   *
   * @param configuration Unitree SDK abstraction configuration.
   * @param state_manager Optional state manager updated from SDK state callbacks.
   */
  explicit UnitreeG1Adapter(
      humanoid::plugins::unitree::sdk::SdkConfiguration configuration,
      std::shared_ptr<humanoid::core::RobotStateManager> state_manager = nullptr);

  /**
   * @brief Destroys the adapter and releases SDK resources.
   */
  ~UnitreeG1Adapter() noexcept override;

  UnitreeG1Adapter(const UnitreeG1Adapter&) = delete;
  UnitreeG1Adapter& operator=(const UnitreeG1Adapter&) = delete;
  UnitreeG1Adapter(UnitreeG1Adapter&&) = delete;
  UnitreeG1Adapter& operator=(UnitreeG1Adapter&&) = delete;

  /**
   * @brief Initializes SDK abstraction resources.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Initialize() override;

  /**
   * @brief Releases SDK abstraction resources.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Shutdown() override;

  /**
   * @brief Establishes or verifies Unitree G1 communication.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Connect() override;

  /**
   * @brief Disconnects robot communication.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Disconnect() override;

  /**
   * @brief Reports whether robot communication is established.
   *
   * @return True when connected.
   */
  [[nodiscard]] bool IsConnected() const noexcept override;

  /**
   * @brief Returns latest normalized robot state.
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
   * @brief Returns adapter capability metadata.
   *
   * @return Vendor-independent capability declaration.
   */
  [[nodiscard]] humanoid::core::RobotCapabilities GetCapabilities() const override;

  /**
   * @brief Returns command capabilities supported by this adapter.
   *
   * @return Command capability set.
   */
  [[nodiscard]] humanoid::core::CommandCapabilitySet GetCommandCapabilities() const override;

  /**
   * @brief Executes one generic command through the Unitree SDK abstraction.
   *
   * @param command Generic command.
   * @return Command execution result.
   */
  [[nodiscard]] humanoid::core::CommandResult
  ExecuteCommand(const humanoid::core::Command& command) override;

  /**
   * @brief Performs one non-blocking SDK state synchronization cycle.
   *
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status Update() override;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::plugins::unitree::g1
