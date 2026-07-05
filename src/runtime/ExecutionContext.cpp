#include <humanoid/runtime/ExecutionContext.h>

#include <utility>

namespace humanoid::runtime {

ExecutionContext::ExecutionContext(ExecutionContextId execution_id, ExecutionScope scope,
                                   ExecutionMissionId mission_id, ExecutionMetadata metadata)
    : execution_id_(execution_id), scope_(scope), mission_id_(mission_id),
      metadata_(std::move(metadata)) {}

ExecutionContextId ExecutionContext::Id() const noexcept { return execution_id_; }

ExecutionScope ExecutionContext::Scope() const noexcept { return scope_; }

ExecutionMissionId ExecutionContext::MissionId() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return mission_id_;
}

void ExecutionContext::SetMissionId(ExecutionMissionId mission_id) {
  std::lock_guard<std::mutex> lock{mutex_};
  mission_id_ = mission_id;
}

ExecutionState ExecutionContext::State() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return state_;
}

void ExecutionContext::SetState(ExecutionState state) {
  std::lock_guard<std::mutex> lock{mutex_};
  state_ = state;
}

std::optional<ExecutionTimestamp> ExecutionContext::StartTimestamp() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return start_timestamp_;
}

void ExecutionContext::SetStartTimestamp(ExecutionTimestamp timestamp) {
  std::lock_guard<std::mutex> lock{mutex_};
  start_timestamp_ = timestamp;
}

void ExecutionContext::ClearStartTimestamp() {
  std::lock_guard<std::mutex> lock{mutex_};
  start_timestamp_.reset();
}

std::optional<ExecutionStepId> ExecutionContext::CurrentStep() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return current_step_;
}

void ExecutionContext::SetCurrentStep(ExecutionStepId step_id) {
  std::lock_guard<std::mutex> lock{mutex_};
  current_step_ = step_id;
}

void ExecutionContext::ClearCurrentStep() {
  std::lock_guard<std::mutex> lock{mutex_};
  current_step_.reset();
}

bool ExecutionContext::RequestCancellation() noexcept {
  return cancellation_source_.request_stop();
}

std::stop_token ExecutionContext::CancellationToken() const noexcept {
  return cancellation_source_.get_token();
}

bool ExecutionContext::CancellationRequested() const noexcept {
  return cancellation_source_.stop_requested();
}

ExecutionMetadata ExecutionContext::Metadata() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return metadata_;
}

void ExecutionContext::SetMetadata(ExecutionMetadata metadata) {
  std::lock_guard<std::mutex> lock{mutex_};
  metadata_ = std::move(metadata);
}

void ExecutionContext::SetMetadataValue(std::string key, std::string value) {
  std::lock_guard<std::mutex> lock{mutex_};
  metadata_.insert_or_assign(std::move(key), std::move(value));
}

std::optional<std::string> ExecutionContext::MetadataValue(std::string_view key) const {
  std::lock_guard<std::mutex> lock{mutex_};
  const auto iterator = metadata_.find(key);
  if (iterator == metadata_.end()) {
    return std::nullopt;
  }
  return iterator->second;
}

bool ExecutionContext::RemoveMetadata(std::string_view key) {
  std::lock_guard<std::mutex> lock{mutex_};
  const auto iterator = metadata_.find(key);
  if (iterator == metadata_.end()) {
    return false;
  }
  metadata_.erase(iterator);
  return true;
}

ExecutionContextSnapshot ExecutionContext::Snapshot() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return ExecutionContextSnapshot{execution_id_,
                                  mission_id_,
                                  state_,
                                  start_timestamp_,
                                  current_step_,
                                  scope_,
                                  cancellation_source_.stop_requested(),
                                  metadata_};
}

} // namespace humanoid::runtime
