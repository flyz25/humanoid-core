#include <humanoid/safety/SafetyManager.hpp>

#include <utility>

namespace humanoid::safety {
namespace {

/**
 * @brief Creates the status returned when no safety controller is attached.
 *
 * @return Failed precondition status.
 */
common::Status missingControllerStatus() {
  return common::Status::error(common::StatusCode::kFailedPrecondition,
                               "safety controller interface is not set");
}

} // namespace

SafetyManager::SafetyManager(std::shared_ptr<SafetyController> controller)
    : controller_(std::move(controller)) {}

common::Status SafetyManager::setController(std::shared_ptr<SafetyController> controller) {
  if (!controller) {
    return common::Status::error(common::StatusCode::kInvalidArgument,
                                 "safety controller interface is null");
  }

  controller_ = std::move(controller);
  return common::Status::ok();
}

void SafetyManager::clearController() noexcept { controller_.reset(); }

bool SafetyManager::hasController() const noexcept { return static_cast<bool>(controller_); }

common::LifecycleState SafetyManager::lifecycleState() const noexcept {
  return controller_ ? controller_->lifecycleState() : common::LifecycleState::kUnconfigured;
}

SafetyState SafetyManager::safetyState() const noexcept {
  return controller_ ? controller_->safetyState() : SafetyState::kUnknown;
}

bool SafetyManager::isMotionAllowed() const noexcept {
  return controller_ ? controller_->isMotionAllowed() : false;
}

common::Status SafetyManager::engageStop(std::string_view reason) {
  if (reason.empty()) {
    return common::Status::error(common::StatusCode::kInvalidArgument,
                                 "safety stop reason is empty");
  }

  return controller_ ? controller_->engageStop(reason) : missingControllerStatus();
}

common::Status SafetyManager::releaseStop() {
  return controller_ ? controller_->releaseStop() : missingControllerStatus();
}

} // namespace humanoid::safety
