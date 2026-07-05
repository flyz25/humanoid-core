#pragma once

/**
 * @file MissionCondition.h
 * @brief Defines vendor-independent mission execution conditions.
 */

#include <cstdint>
#include <string>

#include <humanoid/core/CommandType.h>
#include <humanoid/mission/MissionMetadata.h>

namespace humanoid::mission {

/**
 * @brief Stable mission condition identifier type.
 */
using MissionConditionId = std::uint64_t;

/**
 * @brief State source inspected by a mission condition.
 */
enum class MissionConditionType : std::uint8_t {
  BatteryLevel,
  Connection,
  RobotStanding,
  RobotWalking,
  RobotSitting,
  Capability,
  FaultCode,
  EmergencyStop
};

/**
 * @brief Comparison applied to the selected condition value.
 */
enum class MissionConditionComparison : std::uint8_t {
  Equal,
  NotEqual,
  LessThan,
  LessThanOrEqual,
  GreaterThan,
  GreaterThanOrEqual
};

/**
 * @brief Mission behavior requested when a condition is not satisfied.
 */
enum class MissionConditionFailureAction : std::uint8_t { Skip, Abort };

/**
 * @brief Converts a condition type to a stable diagnostic string.
 *
 * @param type Condition type.
 * @return Stable string name.
 */
[[nodiscard]] constexpr const char* toString(MissionConditionType type) noexcept {
  switch (type) {
  case MissionConditionType::BatteryLevel:
    return "BatteryLevel";
  case MissionConditionType::Connection:
    return "Connection";
  case MissionConditionType::RobotStanding:
    return "RobotStanding";
  case MissionConditionType::RobotWalking:
    return "RobotWalking";
  case MissionConditionType::RobotSitting:
    return "RobotSitting";
  case MissionConditionType::Capability:
    return "Capability";
  case MissionConditionType::FaultCode:
    return "FaultCode";
  case MissionConditionType::EmergencyStop:
    return "EmergencyStop";
  }
  return "Unknown";
}

/**
 * @brief Defines one mission condition evaluated against generic robot state.
 *
 * The condition contains no SDK types and no robot-specific logic. Numeric
 * conditions use `numericValue`, boolean conditions use `boolValue`, fault
 * conditions use `faultCode`, and capability conditions use `commandType`.
 */
struct MissionCondition final {
  /**
   * @brief Unique mission-local condition identifier; zero is unassigned.
   */
  MissionConditionId id{0U};

  /**
   * @brief Human-readable condition name.
   */
  std::string name;

  /**
   * @brief Robot state or capability source inspected by this condition.
   */
  MissionConditionType type{MissionConditionType::Connection};

  /**
   * @brief Comparison applied to the selected value.
   */
  MissionConditionComparison comparison{MissionConditionComparison::Equal};

  /**
   * @brief Numeric comparison value for battery and other scalar conditions.
   */
  float numericValue{0.0F};

  /**
   * @brief Expected boolean value for connection, posture, capability, and emergency stop checks.
   */
  bool boolValue{true};

  /**
   * @brief Expected or threshold fault code for fault-code conditions.
   */
  std::int32_t faultCode{0};

  /**
   * @brief Generic command capability inspected by capability conditions.
   */
  humanoid::core::CommandType commandType{humanoid::core::CommandType::Custom};

  /**
   * @brief Mission behavior when this condition is not satisfied.
   */
  MissionConditionFailureAction failureAction{MissionConditionFailureAction::Abort};

  /**
   * @brief Non-operational annotations for tracing and authoring context.
   */
  MissionMetadata metadata;

  /**
   * @brief Constructs an unassigned connection condition.
   */
  MissionCondition() = default;

  /**
   * @brief Reports whether this condition has a valid identity.
   *
   * @return True when the condition identifier is nonzero.
   */
  [[nodiscard]] bool isValid() const noexcept { return id != 0U; }
};

} // namespace humanoid::mission
