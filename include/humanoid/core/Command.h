#pragma once

/**
 * @file Command.h
 * @brief Defines the generic, vendor-independent robot command model.
 */

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <variant>

#include <humanoid/core/CommandPriority.h>
#include <humanoid/core/CommandType.h>

namespace humanoid::core {

/**
 * @brief Stable command identifier type.
 *
 * A value of zero is reserved for an unassigned identifier. The application or
 * future command producer owns identifier generation and must provide a unique,
 * nonzero value within its command-processing domain.
 */
using CommandId = std::uint64_t;

/**
 * @brief Monotonic creation timestamp used by commands.
 */
using CommandTimestamp =
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

/**
 * @brief Duration type used to express command execution timeouts.
 */
using CommandTimeout = std::chrono::milliseconds;

/**
 * @brief Supported scalar value types for generic command parameters.
 *
 * Parameter names and units belong to the framework-level command contract.
 * Vendor adapters translate those values internally and must not place SDK
 * objects or vendor enums in a payload.
 */
using CommandPayloadValue = std::variant<bool, std::int64_t, double, std::string>;

/**
 * @brief Ordered collection of named command parameters.
 *
 * An ordered map provides deterministic traversal for diagnostics,
 * serialization boundaries, and tests. The transparent comparator permits
 * allocation-free lookup with compatible string-like key types.
 */
using CommandPayload = std::map<std::string, CommandPayloadValue, std::less<>>;

/**
 * @brief Ordered collection of non-operational command annotations.
 *
 * Metadata may carry correlation, tracing, or source information. Command
 * execution must derive operational parameters from `Command::payload`, not
 * from metadata.
 */
using CommandMetadata = std::map<std::string, std::string, std::less<>>;

/**
 * @brief Generic command exchanged across vendor-independent framework layers.
 *
 * `Command` is a framework-owned value type. It contains no SDK headers,
 * vendor enums, transport handles, execution logic, or global identifier
 * generator. A default-constructed command is intentionally unassigned and is
 * not valid for submission until a producer assigns a nonzero identifier.
 */
struct Command final {
  /**
   * @brief Unique, producer-assigned command identifier; zero is unassigned.
   */
  CommandId id{0U};

  /**
   * @brief Monotonic time at which the command was created.
   */
  CommandTimestamp timestamp{};

  /**
   * @brief Framework-level command category.
   */
  CommandType type{CommandType::Custom};

  /**
   * @brief Relative scheduling importance of the command.
   */
  CommandPriority priority{CommandPriority::Normal};

  /**
   * @brief Maximum execution duration; zero disables timeout enforcement.
   *
   * Negative values are invalid and must be rejected before command execution.
   */
  CommandTimeout timeout{CommandTimeout::zero()};

  /**
   * @brief Named, vendor-independent operational command parameters.
   */
  CommandPayload payload;

  /**
   * @brief Non-operational correlation and tracing annotations.
   */
  CommandMetadata metadata;

  /**
   * @brief Constructs an unassigned custom command with normal priority.
   */
  Command() noexcept = default;

  /**
   * @brief Reports whether the command has a valid identity and timeout.
   *
   * @return True when the identifier is nonzero and timeout is nonnegative.
   */
  [[nodiscard]] constexpr bool isValid() const noexcept {
    return id != 0U && timeout >= CommandTimeout::zero();
  }

  /**
   * @brief Reports whether timeout enforcement is requested.
   *
   * @return True when timeout is greater than zero.
   */
  [[nodiscard]] constexpr bool hasTimeout() const noexcept {
    return timeout > CommandTimeout::zero();
  }
};

} // namespace humanoid::core
