#pragma once

/**
 * @file Status.hpp
 * @brief Defines framework status codes and status values.
 */

#include <string>
#include <string_view>

namespace humanoid::common {

/**
 * @brief Identifies the outcome category for a framework operation.
 */
enum class StatusCode {
  kOk,
  kCancelled,
  kInvalidArgument,
  kUnavailable,
  kFailedPrecondition,
  kInternalError
};

/**
 * @brief Converts a status code to a stable string representation.
 *
 * @param code Status code to convert.
 * @return Human-readable status code name.
 */
[[nodiscard]] std::string_view toString(StatusCode code) noexcept;

/**
 * @brief Represents the result of an operation without binding the API to exceptions.
 */
class Status final {
public:
  /**
   * @brief Creates a successful status.
   *
   * @return Status with StatusCode::kOk.
   */
  [[nodiscard]] static Status ok();

  /**
   * @brief Creates an error status.
   *
   * @param code Non-success status code.
   * @param message Diagnostic message for the caller.
   * @return Status carrying the supplied code and message.
   */
  [[nodiscard]] static Status error(StatusCode code, std::string message);

  /**
   * @brief Constructs a successful status.
   */
  Status() = default;

  /**
   * @brief Constructs a status value.
   *
   * @param code Status code.
   * @param message Diagnostic message. Successful statuses store an empty message.
   */
  Status(StatusCode code, std::string message);

  /**
   * @brief Returns the status code.
   *
   * @return Operation status code.
   */
  [[nodiscard]] StatusCode code() const noexcept;

  /**
   * @brief Reports whether the status represents success.
   *
   * @return True when the status code is StatusCode::kOk.
   */
  [[nodiscard]] bool isOk() const noexcept;

  /**
   * @brief Returns the diagnostic message.
   *
   * @return Message associated with the status.
   */
  [[nodiscard]] const std::string& message() const noexcept;

private:
  StatusCode code_{StatusCode::kOk};
  std::string message_;
};

} // namespace humanoid::common
