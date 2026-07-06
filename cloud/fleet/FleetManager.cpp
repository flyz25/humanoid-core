#include <humanoid/cloud/fleet/FleetManager.h>

#include <algorithm>
#include <chrono>
#include <mutex>
#include <utility>

namespace humanoid::cloud::fleet {
namespace {

[[nodiscard]] humanoid::cloud::CloudTimestamp now() noexcept {
  return humanoid::cloud::CloudTimestamp{std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch())};
}

} // namespace

humanoid::cloud::CloudResult FleetManager::RegisterRobot(RobotRecord record) {
  if (record.robotId.empty()) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::InvalidInput,
                                    "robot id is required");
  }

  std::unique_lock lock{mutex_};
  if (robots_.contains(record.robotId)) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::AlreadyExists,
                                    "robot is already registered");
  }

  record.lastHeartbeat = now();
  robots_.emplace(record.robotId, std::move(record));
  return humanoid::cloud::Success("robot registered");
}

humanoid::cloud::CloudResult FleetManager::UnregisterRobot(const std::string& robot_id) {
  std::unique_lock lock{mutex_};
  if (robots_.erase(robot_id) == 0U) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::NotFound,
                                    "robot is not registered");
  }

  for (auto& group : groups_) {
    group.second.erase(robot_id);
  }

  return humanoid::cloud::Success("robot unregistered");
}

humanoid::cloud::CloudResult FleetManager::Heartbeat(const std::string& robot_id, bool healthy) {
  std::unique_lock lock{mutex_};
  const auto robot = robots_.find(robot_id);
  if (robot == robots_.end()) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::NotFound,
                                    "robot is not registered");
  }

  robot->second.online = true;
  robot->second.healthy = healthy;
  robot->second.lastHeartbeat = now();
  return humanoid::cloud::Success("heartbeat accepted");
}

humanoid::cloud::CloudResult FleetManager::AddRobotToGroup(const std::string& group_id,
                                                           const std::string& robot_id) {
  if (group_id.empty()) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::InvalidInput,
                                    "group id is required");
  }

  std::unique_lock lock{mutex_};
  if (!robots_.contains(robot_id)) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::NotFound,
                                    "robot is not registered");
  }

  groups_[group_id].insert(robot_id);
  return humanoid::cloud::Success("robot added to group");
}

humanoid::cloud::CloudResult FleetManager::DistributeMission(const std::string& group_id,
                                                             std::string mission_id) {
  if (mission_id.empty()) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::InvalidInput,
                                    "mission id is required");
  }

  std::unique_lock lock{mutex_};
  const auto group = groups_.find(group_id);
  if (group == groups_.end()) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::NotFound,
                                    "group is not registered");
  }

  MissionDistribution distribution{};
  distribution.missionId = std::move(mission_id);
  distribution.targetRobotIds.assign(group->second.begin(), group->second.end());
  std::sort(distribution.targetRobotIds.begin(), distribution.targetRobotIds.end());
  distribution.timestamp = now();
  distributions_.push_back(std::move(distribution));
  return humanoid::cloud::Success("mission distributed");
}

std::optional<RobotRecord> FleetManager::FindRobot(const std::string& robot_id) const {
  std::shared_lock lock{mutex_};
  const auto robot = robots_.find(robot_id);
  if (robot == robots_.end()) {
    return std::nullopt;
  }

  return robot->second;
}

std::vector<RobotRecord> FleetManager::Robots() const {
  std::shared_lock lock{mutex_};
  std::vector<RobotRecord> robots;
  robots.reserve(robots_.size());
  for (const auto& robot : robots_) {
    robots.push_back(robot.second);
  }

  std::sort(robots.begin(), robots.end(), [](const RobotRecord& lhs, const RobotRecord& rhs) {
    return lhs.robotId < rhs.robotId;
  });
  return robots;
}

FleetStatus FleetManager::Status() const {
  std::shared_lock lock{mutex_};
  FleetStatus status{};
  status.robotCount = robots_.size();
  status.groupCount = groups_.size();

  for (const auto& robot_entry : robots_) {
    if (robot_entry.second.online) {
      ++status.onlineCount;
    }
    if (!robot_entry.second.healthy) {
      ++status.unhealthyCount;
    }
  }

  return status;
}

std::vector<MissionDistribution> FleetManager::MissionDistributions() const {
  std::shared_lock lock{mutex_};
  return distributions_;
}

} // namespace humanoid::cloud::fleet
