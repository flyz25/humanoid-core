#include <humanoid/robot/RobotManager.hpp>

#include <utility>

namespace humanoid::robot {
namespace {

/**
 * @brief Creates the status returned when no robot is attached.
 *
 * @return Failed precondition status.
 */
common::Status missingRobotStatus() {
  return common::Status::error(common::StatusCode::kFailedPrecondition,
                               "robot interface is not set");
}

/**
 * @brief Creates the status returned when no adapter is attached.
 *
 * @return Failed precondition status.
 */
common::Status missingAdapterStatus() {
  return common::Status::error(common::StatusCode::kFailedPrecondition,
                               "robot adapter interface is not set");
}

} // namespace

RobotManager::RobotManager(std::shared_ptr<Robot> robot) : robot_(std::move(robot)) {}

common::Status RobotManager::setRobot(std::shared_ptr<Robot> robot) {
  if (!robot) {
    return common::Status::error(common::StatusCode::kInvalidArgument, "robot interface is null");
  }

  robot_ = std::move(robot);
  return common::Status::ok();
}

void RobotManager::clearRobot() noexcept { robot_.reset(); }

bool RobotManager::hasRobot() const noexcept { return static_cast<bool>(robot_); }

common::LifecycleState RobotManager::lifecycleState() const noexcept {
  return robot_ ? robot_->lifecycleState() : common::LifecycleState::kUnconfigured;
}

common::Status RobotManager::configure() {
  return robot_ ? robot_->configure() : missingRobotStatus();
}

common::Status RobotManager::activate() {
  return robot_ ? robot_->activate() : missingRobotStatus();
}

common::Status RobotManager::deactivate() {
  return robot_ ? robot_->deactivate() : missingRobotStatus();
}

common::Status RobotManager::shutdown() {
  return robot_ ? robot_->shutdown() : missingRobotStatus();
}

common::Status RobotManager::setAdapter(std::shared_ptr<IRobotAdapter> adapter) {
  if (!adapter) {
    return common::Status::error(common::StatusCode::kInvalidArgument,
                                 "robot adapter interface is null");
  }

  adapter_ = std::move(adapter);
  return common::Status::ok();
}

void RobotManager::clearAdapter() noexcept { adapter_.reset(); }

bool RobotManager::hasAdapter() const noexcept { return static_cast<bool>(adapter_); }

common::Status RobotManager::initializeAdapter() {
  return adapter_ ? adapter_->initialize() : missingAdapterStatus();
}

common::Status RobotManager::startAdapter() {
  return adapter_ ? adapter_->start() : missingAdapterStatus();
}

common::Status RobotManager::stopAdapter() {
  return adapter_ ? adapter_->stop() : missingAdapterStatus();
}

common::Status RobotManager::shutdownAdapter() {
  return adapter_ ? adapter_->shutdown() : missingAdapterStatus();
}

} // namespace humanoid::robot
