#pragma once

/**
 * @file MissionStep.h
 * @brief Defines a vendor-independent executable mission step.
 */

#include <chrono>
#include <cstdint>
#include <string>

#include <humanoid/core/Command.h>
#include <humanoid/mission/MissionMetadata.h>

namespace humanoid::mission {

/**
 * @brief Stable mission step identifier type.
 *
 * A value of zero is reserved for an unassigned identifier. Mission authoring
 * tools or application code own identifier generation.
 */
using MissionStepId = std::uint64_t;

/**
 * @brief Duration type used to express mission step timeouts.
 */
using MissionStepTimeout = std::chrono::milliseconds;

/**
 * @brief Retry count type for mission steps.
 */
using MissionStepRetryCount = std::uint32_t;

/**
 * @brief Defines one executable step in a mission.
 *
 * `MissionStep` references the generic command model and contains no adapter,
 * SDK, transport, planning, or execution logic. A default-constructed step is
 * intentionally unassigned and is not valid for execution until it receives a
 * nonzero identifier and a valid command.
 */
struct MissionStep final {
  /**
   * @brief Unique, mission-local step identifier; zero is unassigned.
   */
  MissionStepId id{0U};

  /**
   * @brief Human-readable step name.
   */
  std::string name;

  /**
   * @brief Vendor-independent command requested by this step.
   */
  humanoid::core::Command command;

  /**
   * @brief Maximum step execution duration; zero disables step timeout policy.
   *
   * Negative values are invalid and must be rejected before mission execution.
   */
  MissionStepTimeout timeout{MissionStepTimeout::zero()};

  /**
   * @brief Number of retry attempts permitted after a failed step execution.
   */
  MissionStepRetryCount retry{0U};

  /**
   * @brief Indicates whether this step is eligible for execution.
   */
  bool enabled{true};

  /**
   * @brief Non-operational step annotations for tracing and authoring context.
   */
  MissionMetadata metadata;

  /**
   * @brief Constructs an unassigned enabled mission step.
   */
  MissionStep() = default;

  /**
   * @brief Reports whether the step has a valid identity, command, and timeout.
   *
   * @return True when the step identifier is nonzero, the command is valid, and
   * the timeout is nonnegative.
   */
  [[nodiscard]] constexpr bool isValid() const noexcept {
    return id != 0U && command.isValid() && timeout >= MissionStepTimeout::zero();
  }

  /**
   * @brief Reports whether timeout enforcement is requested for this step.
   *
   * @return True when timeout is greater than zero.
   */
  [[nodiscard]] constexpr bool hasTimeout() const noexcept {
    return timeout > MissionStepTimeout::zero();
  }
};

} // namespace humanoid::mission
