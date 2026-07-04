#pragma once

/**
 * @file LocoAdapter.h
 * @brief Defines the Unitree G1 locomotion SDK adapter.
 */

#include <memory>

#include "SdkTypes.h"

namespace humanoid::plugins::unitree::sdk {

/**
 * @brief Thread-safe adapter that translates locomotion commands into Unitree SDK2 calls.
 *
 * LocoAdapter owns the Unitree G1 locomotion client internally. It exposes only
 * humanoid-core SDK abstraction types and never leaks vendor SDK types.
 */
class LocoAdapter final {
public:
  /**
   * @brief Constructs an uninitialized locomotion adapter.
   */
  LocoAdapter();

  /**
   * @brief Releases the owned SDK client without commanding robot motion.
   */
  ~LocoAdapter() noexcept;

  LocoAdapter(const LocoAdapter&) = delete;
  LocoAdapter& operator=(const LocoAdapter&) = delete;
  LocoAdapter(LocoAdapter&&) = delete;
  LocoAdapter& operator=(LocoAdapter&&) = delete;

  /**
   * @brief Initializes the Unitree SDK2 locomotion client.
   *
   * @param configuration SDK configuration.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Initialize(const SdkConfiguration& configuration);

  /**
   * @brief Releases the Unitree SDK2 locomotion client.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Shutdown();

  /**
   * @brief Verifies communication with the locomotion service.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Connect();

  /**
   * @brief Disconnects local adapter state without commanding motion.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Disconnect();

  /**
   * @brief Reads the current Unitree locomotion FSM id.
   *
   * @param fsm_id Output FSM identifier.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult GetFsmId(std::int32_t& fsm_id);

  /**
   * @brief Commands Unitree stand-up behavior.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Stand();

  /**
   * @brief Commands Unitree balance standing behavior.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult BalanceStand();

  /**
   * @brief Commands Unitree sitting behavior.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Sit();

  /**
   * @brief Commands walking with velocity values.
   *
   * @param velocity Velocity command.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Walk(const SdkVelocityCommand& velocity);

  /**
   * @brief Sends a raw velocity command.
   *
   * @param velocity Velocity command.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult SetVelocity(const SdkVelocityCommand& velocity);

  /**
   * @brief Stops active velocity motion.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Stop();

  /**
   * @brief Commands an emergency stop through StopMove and Damp.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult EmergencyStop();

  /**
   * @brief Reports whether the SDK client is initialized.
   *
   * @return True when initialized.
   */
  [[nodiscard]] bool IsInitialized() const noexcept;

  /**
   * @brief Reports whether communication has been verified.
   *
   * @return True when connected.
   */
  [[nodiscard]] bool IsConnected() const noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::plugins::unitree::sdk
