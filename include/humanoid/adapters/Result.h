#pragma once

/**
 * @file Result.h
 * @brief Defines framework adapter command results.
 */

#include <string>

namespace humanoid::adapters {

/**
 * @brief Error categories returned by robot adapters and SDK wrappers.
 */
enum class ErrorCode {
  kSuccess,
  kSDKUnavailable,
  kConnectionFailed,
  kTimeout,
  kRobotFault,
  kUnknown
};

/**
 * @brief Result object returned by adapter and SDK wrapper operations.
 */
struct Result final {
  /**
   * @brief Operation outcome code.
   */
  ErrorCode code{ErrorCode::kSuccess};

  /**
   * @brief Human-readable diagnostic message.
   */
  std::string message;

  /**
   * @brief Reports whether the operation succeeded.
   *
   * @return True when the result code is ErrorCode::kSuccess.
   */
  [[nodiscard]] bool Succeeded() const noexcept { return code == ErrorCode::kSuccess; }
};

} // namespace humanoid::adapters
