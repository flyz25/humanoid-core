#include "TelemetryService.h"

#include <limits>
#include <utility>

namespace humanoid::services {

TelemetryService::TelemetryService(std::shared_ptr<const core::RobotStateManager> state_manager,
                                   std::chrono::milliseconds polling_interval) noexcept
    : state_manager_(std::move(state_manager)),
      polling_interval_(NormalizePollingInterval(polling_interval)) {}

TelemetryService::~TelemetryService() { Stop(); }

bool TelemetryService::Start() {
  std::lock_guard<std::mutex> lock{lifecycle_mutex_};
  if (!state_manager_ || worker_.joinable()) {
    return false;
  }

  stop_requested_ = false;

  try {
    worker_ = std::jthread{[this](std::stop_token stop_token) { Run(stop_token); }};
  } catch (...) {
    stop_requested_ = true;
    return false;
  }

  return true;
}

void TelemetryService::Stop() {
  std::jthread worker_to_join;

  {
    std::lock_guard<std::mutex> lock{lifecycle_mutex_};
    stop_requested_ = true;

    if (!worker_.joinable()) {
      lifecycle_condition_.notify_all();
      return;
    }

    worker_.request_stop();
    lifecycle_condition_.notify_all();

    if (worker_.get_id() == std::this_thread::get_id()) {
      return;
    }

    worker_to_join = std::move(worker_);
  }

  lifecycle_condition_.notify_all();
}

TelemetryService::SubscriptionId TelemetryService::Subscribe(Listener listener) {
  if (!listener) {
    return kInvalidSubscriptionId;
  }

  std::unique_lock<std::shared_mutex> lock{listeners_mutex_};
  const SubscriptionId subscription_id = AllocateSubscriptionIdLocked();
  if (subscription_id == kInvalidSubscriptionId) {
    return kInvalidSubscriptionId;
  }

  try {
    listeners_.emplace(subscription_id, std::move(listener));
  } catch (...) {
    return kInvalidSubscriptionId;
  }

  return subscription_id;
}

bool TelemetryService::Unsubscribe(SubscriptionId subscription_id) {
  if (subscription_id == kInvalidSubscriptionId) {
    return false;
  }

  std::unique_lock<std::shared_mutex> lock{listeners_mutex_};
  return listeners_.erase(subscription_id) > 0;
}

void TelemetryService::Run(std::stop_token stop_token) {
  while (!stop_token.stop_requested()) {
    try {
      PublishCurrentState();
    } catch (...) {
    }

    std::unique_lock<std::mutex> lock{lifecycle_mutex_};
    lifecycle_condition_.wait_for(lock, polling_interval_, [this, &stop_token]() {
      return stop_requested_ || stop_token.stop_requested();
    });

    if (stop_requested_ || stop_token.stop_requested()) {
      break;
    }
  }
}

void TelemetryService::PublishCurrentState() const {
  const core::RobotState state = state_manager_->GetState();

  std::vector<Listener> listeners;
  {
    std::shared_lock<std::shared_mutex> lock{listeners_mutex_};
    listeners.reserve(listeners_.size());
    for (const auto& [subscription_id, listener] : listeners_) {
      (void)subscription_id;
      listeners.push_back(listener);
    }
  }

  for (const Listener& listener : listeners) {
    try {
      listener(state);
    } catch (...) {
    }
  }
}

TelemetryService::SubscriptionId TelemetryService::AllocateSubscriptionIdLocked() {
  constexpr SubscriptionId kMaxSubscriptionId = std::numeric_limits<SubscriptionId>::max();

  for (SubscriptionId attempts = 0; attempts < kMaxSubscriptionId; ++attempts) {
    const SubscriptionId candidate = next_subscription_id_;
    ++next_subscription_id_;
    if (next_subscription_id_ == kInvalidSubscriptionId) {
      next_subscription_id_ = 1;
    }

    if (candidate != kInvalidSubscriptionId && listeners_.find(candidate) == listeners_.end()) {
      return candidate;
    }
  }

  return kInvalidSubscriptionId;
}

} // namespace humanoid::services
