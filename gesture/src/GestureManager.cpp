#include <humanoid/gesture/GestureManager.hpp>

#include <utility>

namespace humanoid::gesture {
namespace {

/**
 * @brief Creates the status returned when no gesture controller is attached.
 *
 * @return Failed precondition status.
 */
common::Status missingControllerStatus() {
  return common::Status::error(common::StatusCode::kFailedPrecondition,
                               "gesture controller interface is not set");
}

} // namespace

GestureManager::GestureManager(std::shared_ptr<GestureController> controller)
    : controller_(std::move(controller)) {}

common::Status GestureManager::setController(std::shared_ptr<GestureController> controller) {
  if (!controller) {
    return common::Status::error(common::StatusCode::kInvalidArgument,
                                 "gesture controller interface is null");
  }

  controller_ = std::move(controller);
  return common::Status::ok();
}

void GestureManager::clearController() noexcept { controller_.reset(); }

bool GestureManager::hasController() const noexcept { return static_cast<bool>(controller_); }

common::LifecycleState GestureManager::lifecycleState() const noexcept {
  return controller_ ? controller_->lifecycleState() : common::LifecycleState::kUnconfigured;
}

bool GestureManager::supportsGesture(std::string_view gesture_id) const {
  return controller_ ? controller_->supportsGesture(gesture_id) : false;
}

common::Status GestureManager::startGesture(std::string_view gesture_id) {
  if (gesture_id.empty()) {
    return common::Status::error(common::StatusCode::kInvalidArgument, "gesture id is empty");
  }

  return controller_ ? controller_->startGesture(gesture_id) : missingControllerStatus();
}

common::Status GestureManager::stopGesture() {
  return controller_ ? controller_->stopGesture() : missingControllerStatus();
}

} // namespace humanoid::gesture
