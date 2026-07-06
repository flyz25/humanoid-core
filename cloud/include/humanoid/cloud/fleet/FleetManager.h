#pragma once

/**
 * @file FleetManager.h
 * @brief Defines a thread-safe fleet registry and status manager.
 */

#include <cstddef>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <humanoid/cloud/CloudTypes.h>

namespace humanoid::cloud::fleet {

/**
 * @brief Registered robot description.
 */
struct RobotRecord final {
  /** @brief Stable robot identifier. */
  std::string robotId;

  /** @brief Robot vendor name. */
  std::string vendor;

  /** @brief Robot model name. */
  std::string model;

  /** @brief True when the robot is currently online. */
  bool online{false};

  /** @brief True when the robot reports healthy state. */
  bool healthy{true};

  /** @brief Capability names advertised by the robot. */
  std::vector<std::string> capabilities;

  /** @brief Last heartbeat timestamp. */
  humanoid::cloud::CloudTimestamp lastHeartbeat{};

  /** @brief Non-operational metadata. */
  humanoid::cloud::Metadata metadata;
};

/**
 * @brief Fleet status summary.
 */
struct FleetStatus final {
  /** @brief Total registered robots. */
  std::size_t robotCount{0U};

  /** @brief Robots currently online. */
  std::size_t onlineCount{0U};

  /** @brief Robots currently reporting unhealthy state. */
  std::size_t unhealthyCount{0U};

  /** @brief Number of registered groups. */
  std::size_t groupCount{0U};
};

/**
 * @brief Mission distribution record.
 */
struct MissionDistribution final {
  /** @brief Mission identifier. */
  std::string missionId;

  /** @brief Target robot identifiers. */
  std::vector<std::string> targetRobotIds;

  /** @brief Distribution timestamp. */
  humanoid::cloud::CloudTimestamp timestamp{};
};

/**
 * @brief Thread-safe in-memory fleet registry.
 *
 * The manager owns no network transport and performs no robot communication.
 * Cloud or REST adapters inject requests into this boundary.
 */
class FleetManager final {
public:
  /** @brief Constructs an empty fleet manager. */
  FleetManager() = default;

  /** @brief Destroys the manager. */
  ~FleetManager() = default;

  FleetManager(const FleetManager&) = delete;
  FleetManager& operator=(const FleetManager&) = delete;
  FleetManager(FleetManager&&) = delete;
  FleetManager& operator=(FleetManager&&) = delete;

  /**
   * @brief Registers a robot.
   *
   * @param record Robot record to store.
   * @return Operation result.
   */
  [[nodiscard]] humanoid::cloud::CloudResult RegisterRobot(RobotRecord record);

  /**
   * @brief Removes a robot from the registry and all groups.
   *
   * @param robot_id Robot identifier.
   * @return Operation result.
   */
  [[nodiscard]] humanoid::cloud::CloudResult UnregisterRobot(const std::string& robot_id);

  /**
   * @brief Updates heartbeat and health state for a robot.
   *
   * @param robot_id Robot identifier.
   * @param healthy Current health flag.
   * @return Operation result.
   */
  [[nodiscard]] humanoid::cloud::CloudResult Heartbeat(const std::string& robot_id, bool healthy);

  /**
   * @brief Adds a robot to a named group.
   *
   * @param group_id Group identifier.
   * @param robot_id Robot identifier.
   * @return Operation result.
   */
  [[nodiscard]] humanoid::cloud::CloudResult AddRobotToGroup(const std::string& group_id,
                                                             const std::string& robot_id);

  /**
   * @brief Records mission distribution to a group.
   *
   * @param group_id Group identifier.
   * @param mission_id Mission identifier.
   * @return Operation result.
   */
  [[nodiscard]] humanoid::cloud::CloudResult DistributeMission(const std::string& group_id,
                                                               std::string mission_id);

  /**
   * @brief Returns one robot record.
   *
   * @param robot_id Robot identifier.
   * @return Robot record when present.
   */
  [[nodiscard]] std::optional<RobotRecord> FindRobot(const std::string& robot_id) const;

  /**
   * @brief Returns all robot records.
   *
   * @return Robot records.
   */
  [[nodiscard]] std::vector<RobotRecord> Robots() const;

  /**
   * @brief Returns the latest fleet summary.
   *
   * @return Fleet summary.
   */
  [[nodiscard]] FleetStatus Status() const;

  /**
   * @brief Returns recorded mission distributions.
   *
   * @return Distribution records.
   */
  [[nodiscard]] std::vector<MissionDistribution> MissionDistributions() const;

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, RobotRecord> robots_;
  std::unordered_map<std::string, std::unordered_set<std::string>> groups_;
  std::vector<MissionDistribution> distributions_;
};

} // namespace humanoid::cloud::fleet
