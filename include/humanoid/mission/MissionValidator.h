#pragma once

/**
 * @file MissionValidator.h
 * @brief Defines schema validation for vendor-independent missions.
 */

#include <string>

#include <humanoid/mission/Mission.h>

namespace humanoid::mission {

/**
 * @brief Result returned by mission schema validation.
 */
struct MissionValidationResult final {
  /**
   * @brief True when validation passed.
   */
  bool valid{false};

  /**
   * @brief Human-readable validation diagnostic.
   */
  std::string message;

  /**
   * @brief Reports whether validation passed.
   *
   * @return True when the mission is valid.
   */
  [[nodiscard]] bool Succeeded() const noexcept { return valid; }
};

/**
 * @brief Validates a mission object before execution or storage.
 *
 * The validator is vendor independent and contains no parser, robot adapter, or
 * SDK dependency. It checks the framework mission schema represented by the
 * in-memory `Mission` model.
 */
class MissionValidator final {
public:
  /**
   * @brief Constructs a mission validator.
   */
  MissionValidator() = default;

  /**
   * @brief Validates a mission object.
   *
   * @param mission Mission object to validate.
   * @return Validation result.
   */
  [[nodiscard]] MissionValidationResult Validate(const Mission& mission) const;
};

} // namespace humanoid::mission
