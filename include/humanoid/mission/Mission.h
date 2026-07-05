#pragma once

/**
 * @file Mission.h
 * @brief Defines a vendor-independent collection of executable mission steps.
 */

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <humanoid/mission/MissionMetadata.h>
#include <humanoid/mission/MissionStep.h>

namespace humanoid::mission {

/**
 * @brief Stable mission identifier type.
 *
 * A value of zero is reserved for an unassigned identifier. Mission authoring
 * tools or application code own identifier generation.
 */
using MissionId = std::uint64_t;

/**
 * @brief Monotonic timestamp used to identify mission creation or revision.
 */
using MissionTimestamp =
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

/**
 * @brief Ordered collection of executable mission steps.
 */
using MissionSteps = std::vector<MissionStep>;

/**
 * @brief Generic vendor-independent mission model.
 *
 * A mission is a declarative collection of executable steps. This type contains
 * no parser, scheduler, mission engine, robot adapter, vendor SDK, transport
 * handle, or global identifier generator. Execution policy belongs to future
 * components that consume this model.
 */
struct Mission final {
  /**
   * @brief Unique, producer-assigned mission identifier; zero is unassigned.
   */
  MissionId id{0U};

  /**
   * @brief Human-readable mission name.
   */
  std::string name;

  /**
   * @brief Human-readable mission description.
   */
  std::string description;

  /**
   * @brief Mission document or authoring format version.
   */
  std::string version;

  /**
   * @brief Mission author or producing system name.
   */
  std::string author;

  /**
   * @brief Monotonic timestamp associated with mission creation or revision.
   */
  MissionTimestamp timestamp{};

  /**
   * @brief Ordered executable steps that define the mission.
   */
  MissionSteps steps;

  /**
   * @brief Non-operational mission annotations for tracing and authoring.
   */
  MissionMetadata metadata;

  /**
   * @brief Constructs an unassigned mission with no steps.
   */
  Mission() = default;

  /**
   * @brief Reports whether the mission has a valid identity and enabled steps.
   *
   * Disabled steps are allowed to remain in the model for authoring workflows
   * and are not considered executable. A mission with no enabled steps is not
   * valid for execution.
   *
   * @return True when the mission identifier is nonzero and every enabled step
   * is valid.
   */
  [[nodiscard]] bool isValid() const noexcept {
    bool has_enabled_step = false;
    for (const MissionStep& step : steps) {
      if (!step.enabled) {
        continue;
      }

      has_enabled_step = true;
      if (!step.isValid()) {
        return false;
      }
    }

    return id != 0U && has_enabled_step;
  }

  /**
   * @brief Reports whether the mission contains at least one step.
   *
   * @return True when the step collection is not empty.
   */
  [[nodiscard]] bool hasSteps() const noexcept { return !steps.empty(); }

  /**
   * @brief Returns the number of steps in the mission.
   *
   * @return Total number of steps, including disabled steps.
   */
  [[nodiscard]] std::size_t stepCount() const noexcept { return steps.size(); }
};

} // namespace humanoid::mission
