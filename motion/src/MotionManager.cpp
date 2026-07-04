#include <humanoid/motion/MotionManager.hpp>

#include <utility>

namespace humanoid::motion {
namespace {

/**
 * @brief Creates the status returned when no motion controller is attached.
 *
 * @return Failed precondition status.
 */
common::Status missingControllerStatus() {
  return common::Status::error(common::StatusCode::kFailedPrecondition,
                               "motion controller interface is not set");
}

} // namespace

MotionManager::MotionManager(std::shared_ptr<MotionController> controller)
    : controller_(std::move(controller)) {}

common::Status MotionManager::setController(std::shared_ptr<MotionController> controller) {
  if (!controller) {
    return common::Status::error(common::StatusCode::kInvalidArgument,
                                 "motion controller interface is null");
  }

  controller_ = std::move(controller);
  return common::Status::ok();
}

void MotionManager::clearController() noexcept { controller_.reset(); }

bool MotionManager::hasController() const noexcept { return static_cast<bool>(controller_); }

common::LifecycleState MotionManager::lifecycleState() const noexcept {
  return controller_ ? controller_->lifecycleState() : common::LifecycleState::kUnconfigured;
}

std::optional<MotionMode> MotionManager::motionMode() const noexcept {
  if (!controller_) {
    return std::nullopt;
  }

  return controller_->motionMode();
}

common::Status MotionManager::setMotionMode(MotionMode mode) {
  return controller_ ? controller_->setMotionMode(mode) : missingControllerStatus();
}

common::Status MotionManager::holdPosition() {
  return controller_ ? controller_->holdPosition() : missingControllerStatus();
}

common::Status MotionManager::stopMotion() {
  return controller_ ? controller_->stopMotion() : missingControllerStatus();
}

} // namespace humanoid::motion
