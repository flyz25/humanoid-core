#pragma once

/**
 * @file CloudTypes.h
 * @brief Shared value types for the optional cloud platform layer.
 */

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace humanoid::cloud {

/**
 * @brief Monotonic timestamp used by cloud platform records.
 */
using CloudTimestamp = std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

/**
 * @brief String metadata map used for non-operational annotations.
 */
using Metadata = std::map<std::string, std::string, std::less<>>;

/**
 * @brief Result code for cloud platform operations.
 */
enum class CloudErrorCode : std::uint8_t {
  Success,       ///< Operation completed successfully.
  NotFound,      ///< Requested entity does not exist.
  AlreadyExists, ///< Entity already exists.
  InvalidInput,  ///< Input failed validation.
  Unauthorized,  ///< Principal is not authorized.
  Unavailable,   ///< Optional dependency or backend is unavailable.
  Failed         ///< Operation failed for a non-specific reason.
};

/**
 * @brief Operation result used by cloud platform managers.
 */
struct CloudResult final {
  /** @brief Result code. */
  CloudErrorCode code{CloudErrorCode::Success};

  /** @brief Human-readable diagnostic message. */
  std::string message;

  /**
   * @brief Reports whether the operation succeeded.
   *
   * @return True when `code` is `CloudErrorCode::Success`.
   */
  [[nodiscard]] constexpr bool ok() const noexcept { return code == CloudErrorCode::Success; }
};

/**
 * @brief Creates a successful cloud result.
 *
 * @param message Optional diagnostic message.
 * @return Successful result.
 */
[[nodiscard]] inline CloudResult Success(std::string message = {}) {
  return CloudResult{CloudErrorCode::Success, std::move(message)};
}

/**
 * @brief Creates a failed cloud result.
 *
 * @param code Failure code.
 * @param message Diagnostic message.
 * @return Failed result.
 */
[[nodiscard]] inline CloudResult Failure(CloudErrorCode code, std::string message) {
  return CloudResult{code, std::move(message)};
}

} // namespace humanoid::cloud
