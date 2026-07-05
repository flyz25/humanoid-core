#pragma once

/**
 * @file MissionEvent.h
 * @brief Defines vendor-independent mission events.
 */

#include <chrono>
#include <cstdint>
#include <string>

#include <humanoid/mission/MissionCondition.h>
#include <humanoid/mission/MissionMetadata.h>
#include <humanoid/mission/MissionStep.h>

namespace humanoid::mission {

/**
 * @brief Stable mission event identifier type.
 */
using MissionEventId = std::uint64_t;

/**
 * @brief Monotonic timestamp type used for mission events.
 */
using MissionEventTimestamp =
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

/**
 * @brief Event type emitted by mission condition evaluation.
 */
enum class MissionEventType : std::uint8_t {
  ConditionSatisfied,
  ConditionFailed,
  ConditionUnavailable
};

/**
 * @brief Converts an event type to a stable diagnostic string.
 *
 * @param type Event type.
 * @return Stable string name.
 */
[[nodiscard]] constexpr const char* toString(MissionEventType type) noexcept {
  switch (type) {
  case MissionEventType::ConditionSatisfied:
    return "ConditionSatisfied";
  case MissionEventType::ConditionFailed:
    return "ConditionFailed";
  case MissionEventType::ConditionUnavailable:
    return "ConditionUnavailable";
  }
  return "Unknown";
}

/**
 * @brief Event generated from mission condition evaluation.
 *
 * Events are value objects intended for diagnostics, tests, and future mission
 * observers. They do not transport SDK objects or robot-specific payloads.
 */
struct MissionEvent final {
  /**
   * @brief Event identifier; zero means not assigned by an event store.
   */
  MissionEventId id{0U};

  /**
   * @brief Condition that produced the event.
   */
  MissionConditionId conditionId{0U};

  /**
   * @brief Step associated with the condition when available.
   */
  MissionStepId stepId{0U};

  /**
   * @brief Event classification.
   */
  MissionEventType type{MissionEventType::ConditionUnavailable};

  /**
   * @brief Monotonic event timestamp.
   */
  MissionEventTimestamp timestamp{};

  /**
   * @brief Human-readable diagnostic message.
   */
  std::string message;

  /**
   * @brief Non-operational annotations for tracing and authoring context.
   */
  MissionMetadata metadata;

  /**
   * @brief Constructs an empty condition-unavailable event.
   */
  MissionEvent() = default;
};

} // namespace humanoid::mission
