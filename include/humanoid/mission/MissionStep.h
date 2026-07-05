#pragma once

/**
 * @file MissionStep.h
 * @brief Defines a vendor-independent executable mission step.
 */

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include <humanoid/core/Command.h>
#include <humanoid/mission/DelayStep.h>
#include <humanoid/mission/LoopPolicy.h>
#include <humanoid/mission/MissionMetadata.h>
#include <humanoid/mission/RetryPolicy.h>
#include <humanoid/mission/TimeoutPolicy.h>
#include <humanoid/mission/WaitStep.h>

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
   *
   * This legacy field is preserved for compatibility. New mission documents may
   * prefer `retryPolicy`, where `maxAttempts` includes the initial attempt.
   */
  MissionStepRetryCount retry{0U};

  /**
   * @brief Retry policy applied to this step execution.
   */
  RetryPolicy retryPolicy;

  /**
   * @brief Loop policy applied to this step.
   */
  LoopPolicy loopPolicy;

  /**
   * @brief Timeout policy applied to this step.
   */
  TimeoutPolicy timeoutPolicy;

  /**
   * @brief Optional wait behavior for this step.
   *
   * A wait step contains no robot command. It blocks mission progression for a
   * fixed duration while still allowing cancellation and stop requests.
   */
  std::optional<WaitStep> wait;

  /**
   * @brief Optional delay behavior for this step.
   *
   * A delay step contains no robot command. It is intended for deterministic
   * spacing between command steps.
   */
  std::optional<DelayStep> delay;

  /**
   * @brief Indicates that this enabled step should be skipped intentionally.
   */
  bool skip{false};

  /**
   * @brief Indicates that this step should abort mission execution.
   */
  bool abort{false};

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
   * @brief Reports whether the step has a valid identity and behavior.
   *
   * Command steps require a valid command. Wait, delay, skip, and abort steps do
   * not require a command. All flow-control policies must be valid.
   *
   * @return True when the step can be interpreted by the mission executor.
   */
  [[nodiscard]] constexpr bool isValid() const noexcept {
    if (id == 0U || timeout < MissionStepTimeout::zero() || !retryPolicy.isValid() ||
        !loopPolicy.isValid() || !timeoutPolicy.isValid()) {
      return false;
    }
    if (wait.has_value() && !wait->isValid()) {
      return false;
    }
    if (delay.has_value() && !delay->isValid()) {
      return false;
    }
    if (skip || abort || wait.has_value() || delay.has_value()) {
      return true;
    }
    return command.isValid();
  }

  /**
   * @brief Reports whether timeout enforcement is requested for this step.
   *
   * @return True when timeout is greater than zero.
   */
  [[nodiscard]] constexpr bool hasTimeout() const noexcept {
    return timeout > MissionStepTimeout::zero();
  }

  /**
   * @brief Reports whether timeout policy enforcement is requested.
   *
   * @return True when either legacy timeout or timeout policy is enabled.
   */
  [[nodiscard]] constexpr bool hasEffectiveTimeout() const noexcept {
    return hasTimeout() || timeoutPolicy.isEnabled();
  }
};

} // namespace humanoid::mission
