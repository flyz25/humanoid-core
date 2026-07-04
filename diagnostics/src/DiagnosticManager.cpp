#include <humanoid/diagnostics/DiagnosticManager.hpp>

#include <utility>

namespace humanoid::diagnostics {
namespace {

/**
 * @brief Creates the status returned when no diagnostic controller is attached.
 *
 * @return Failed precondition status.
 */
common::Status missingControllerStatus() {
  return common::Status::error(common::StatusCode::kFailedPrecondition,
                               "diagnostic controller interface is not set");
}

} // namespace

DiagnosticManager::DiagnosticManager(std::shared_ptr<DiagnosticController> controller)
    : controller_(std::move(controller)) {}

common::Status DiagnosticManager::setController(std::shared_ptr<DiagnosticController> controller) {
  if (!controller) {
    return common::Status::error(common::StatusCode::kInvalidArgument,
                                 "diagnostic controller interface is null");
  }

  controller_ = std::move(controller);
  return common::Status::ok();
}

void DiagnosticManager::clearController() noexcept { controller_.reset(); }

bool DiagnosticManager::hasController() const noexcept { return static_cast<bool>(controller_); }

common::LifecycleState DiagnosticManager::lifecycleState() const noexcept {
  return controller_ ? controller_->lifecycleState() : common::LifecycleState::kUnconfigured;
}

std::vector<DiagnosticRecord> DiagnosticManager::collect() const {
  if (!controller_) {
    return {};
  }

  return controller_->collect();
}

common::Status DiagnosticManager::runSelfTest() {
  return controller_ ? controller_->runSelfTest() : missingControllerStatus();
}

} // namespace humanoid::diagnostics
