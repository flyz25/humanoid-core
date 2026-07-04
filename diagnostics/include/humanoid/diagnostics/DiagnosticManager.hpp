#pragma once

/**
 * @file DiagnosticManager.hpp
 * @brief Defines the diagnostic manager.
 */

#include <memory>
#include <vector>

#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>
#include <humanoid/diagnostics/DiagnosticController.hpp>
#include <humanoid/diagnostics/DiagnosticRecord.hpp>

namespace humanoid::diagnostics {

/**
 * @brief Coordinates access to an injected diagnostic controller interface.
 */
class DiagnosticManager final {
public:
  /**
   * @brief Constructs an empty diagnostic manager.
   */
  DiagnosticManager() = default;

  /**
   * @brief Constructs a diagnostic manager with a controller.
   *
   * @param controller Diagnostic controller interface.
   */
  explicit DiagnosticManager(std::shared_ptr<DiagnosticController> controller);

  /**
   * @brief Sets the active diagnostic controller.
   *
   * @param controller Diagnostic controller interface.
   * @return Success when the controller is non-null.
   */
  common::Status setController(std::shared_ptr<DiagnosticController> controller);

  /**
   * @brief Clears the active diagnostic controller.
   */
  void clearController() noexcept;

  /**
   * @brief Reports whether a controller is attached.
   *
   * @return True when a controller interface is attached.
   */
  [[nodiscard]] bool hasController() const noexcept;

  /**
   * @brief Returns the controller lifecycle state.
   *
   * @return Active controller state, or unconfigured when no controller is attached.
   */
  [[nodiscard]] common::LifecycleState lifecycleState() const noexcept;

  /**
   * @brief Collects diagnostic records from the active controller.
   *
   * @return Diagnostic records, or an empty collection when no controller is attached.
   */
  [[nodiscard]] std::vector<DiagnosticRecord> collect() const;

  /**
   * @brief Executes a self-test through the active controller.
   *
   * @return Operation status.
   */
  common::Status runSelfTest();

private:
  std::shared_ptr<DiagnosticController> controller_;
};

} // namespace humanoid::diagnostics
