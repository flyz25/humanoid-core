#include <humanoid/bt/CommandNode.h>

#include <chrono>
#include <exception>
#include <utility>

#include <humanoid/bt/BTContext.h>
#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/core/CommandStatus.h>

namespace humanoid::bt {
namespace {

[[nodiscard]] BTStatus FromCommandStatus(core::CommandStatus status) noexcept {
  switch (status) {
  case core::CommandStatus::Completed:
    return BTStatus::Success;
  case core::CommandStatus::Cancelled:
    return BTStatus::Aborted;
  case core::CommandStatus::Pending:
  case core::CommandStatus::Queued:
  case core::CommandStatus::Running:
    return BTStatus::Running;
  case core::CommandStatus::Failed:
  case core::CommandStatus::Timeout:
  case core::CommandStatus::Rejected:
    return BTStatus::Failure;
  }

  return BTStatus::Failure;
}

} // namespace

CommandNode::CommandNode(std::string name, std::shared_ptr<core::CommandDispatcher> dispatcher,
                         core::Command command)
    : name_(std::move(name)), dispatcher_(std::move(dispatcher)), command_(std::move(command)) {
  if (name_.empty()) {
    name_ = "Command";
  }
}

CommandNode::~CommandNode() noexcept {
  std::lock_guard<std::mutex> lock{mutex_};
  CancelActiveLocked();
}

std::string_view CommandNode::Name() const noexcept { return name_; }

BTStatus CommandNode::Initialize(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  terminal_status_.reset();
  return dispatcher_ ? BTStatus::Idle : BTStatus::Failure;
}

BTStatus CommandNode::Tick(BTContext& context) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (terminal_status_.has_value()) {
    return *terminal_status_;
  }
  if (context.CancellationRequested()) {
    CancelActiveLocked();
    terminal_status_ = BTStatus::Aborted;
    return *terminal_status_;
  }
  if (!dispatcher_) {
    terminal_status_ = BTStatus::Failure;
    return *terminal_status_;
  }

  try {
    if (!future_.has_value()) {
      future_ = dispatcher_->ExecuteAsync(command_);
    }

    if (future_->wait_for(std::chrono::milliseconds{0}) != std::future_status::ready) {
      return BTStatus::Running;
    }

    const core::CommandResult result = future_->get();
    future_.reset();
    terminal_status_ = FromCommandStatus(result.status);
    return *terminal_status_;
  } catch (const std::exception&) {
    future_.reset();
    terminal_status_ = BTStatus::Failure;
    return *terminal_status_;
  } catch (...) {
    future_.reset();
    terminal_status_ = BTStatus::Failure;
    return *terminal_status_;
  }
}

void CommandNode::Reset(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  CancelActiveLocked();
  terminal_status_.reset();
}

void CommandNode::Shutdown(BTContext&) {
  std::lock_guard<std::mutex> lock{mutex_};
  CancelActiveLocked();
}

void CommandNode::CancelActiveLocked() noexcept {
  if (future_.has_value()) {
    if (dispatcher_) {
      try {
        static_cast<void>(dispatcher_->Cancel(command_.id));
      } catch (...) {
      }
    }
    future_.reset();
  }
}

} // namespace humanoid::bt
