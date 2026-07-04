#pragma once

/**
 * @file DiagnosticController.hpp
 * @brief Defines the abstract diagnostic controller interface.
 */

#include <vector>

#include <humanoid/common/LifecycleState.hpp>
#include <humanoid/common/Status.hpp>
#include <humanoid/diagnostics/DiagnosticRecord.hpp>

namespace humanoid::diagnostics {

/**
 * @brief Abstract boundary for diagnostic collection and self-test execution.
 */
class DiagnosticController {
public:
  /**
   * @brief Destroys the diagnostic controller interface.
   */
  virtual ~DiagnosticController() = default;

  /**
   * @brief Returns the controller lifecycle state.
   *
   * @return Controller lifecycle state.
   */
  [[nodiscard]] virtual common::LifecycleState lifecycleState() const noexcept = 0;

  /**
   * @brief Collects diagnostic records.
   *
   * @return Diagnostic records from the controller.
   */
  [[nodiscard]] virtual std::vector<DiagnosticRecord> collect() const = 0;

  /**
   * @brief Executes a self-test.
   *
   * @return Operation status.
   */
  virtual common::Status runSelfTest() = 0;
};

} // namespace humanoid::diagnostics
