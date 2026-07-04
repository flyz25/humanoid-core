#pragma once

/**
 * @file HandAdapter.h
 * @brief Defines the Unitree G1 hand and upper-body action SDK adapter.
 */

#include <memory>

#include "SdkTypes.h"

namespace humanoid::plugins::unitree::sdk {

/**
 * @brief Thread-safe adapter for Unitree G1 hand and upper-body actions.
 *
 * Unitree SDK2 exposes G1 hand-related behavior through the arm action service.
 * This adapter maps supported generic gestures to SDK action ids and rejects
 * unsupported finger-level commands explicitly.
 */
class HandAdapter final {
public:
  /**
   * @brief Constructs an uninitialized hand adapter.
   */
  HandAdapter();

  /**
   * @brief Releases the owned SDK client.
   */
  ~HandAdapter() noexcept;

  HandAdapter(const HandAdapter&) = delete;
  HandAdapter& operator=(const HandAdapter&) = delete;
  HandAdapter(HandAdapter&&) = delete;
  HandAdapter& operator=(HandAdapter&&) = delete;

  /**
   * @brief Initializes the Unitree SDK2 arm action client.
   *
   * @param configuration SDK configuration.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Initialize(const SdkConfiguration& configuration);

  /**
   * @brief Releases the Unitree SDK2 arm action client.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Shutdown();

  /**
   * @brief Verifies communication with the arm action service.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Connect();

  /**
   * @brief Requests a hand-open command when supported by the vendor SDK.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Open();

  /**
   * @brief Requests a hand-close command when supported by the vendor SDK.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Close();

  /**
   * @brief Requests a grip command when supported by the vendor SDK.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Grip();

  /**
   * @brief Releases the Unitree arm action controller.
   *
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Release();

  /**
   * @brief Executes a supported high-level Unitree hand or upper-body gesture.
   *
   * @param gesture Gesture command.
   * @return Operation result.
   */
  [[nodiscard]] SdkResult Gesture(SdkHandGesture gesture);

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
