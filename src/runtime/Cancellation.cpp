#include <humanoid/runtime/Cancellation.h>

#include <cstdint>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

namespace humanoid::runtime {
namespace {

[[nodiscard]] bool IsValidRegistrationId(std::uint64_t id) noexcept { return id != 0U; }

void InvokeSafely(const CancellationCallback& callback) noexcept {
  try {
    if (callback) {
      callback();
    }
  } catch (...) {
  }
}

} // namespace

namespace detail {

class CancellationState final : public std::enable_shared_from_this<CancellationState> {
public:
  [[nodiscard]] bool Cancel() noexcept {
    std::vector<CancellationCallback> callbacks;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      if (cancelled_) {
        return false;
      }

      cancelled_ = true;
      callbacks.reserve(callbacks_.size());
      for (auto& [id, callback] : callbacks_) {
        static_cast<void>(id);
        callbacks.push_back(std::move(callback));
      }
      callbacks_.clear();
    }

    for (const CancellationCallback& callback : callbacks) {
      InvokeSafely(callback);
    }
    return true;
  }

  [[nodiscard]] bool IsCancelled() const noexcept {
    std::lock_guard<std::mutex> lock{mutex_};
    return cancelled_;
  }

  [[nodiscard]] CancellationRegistration Register(CancellationCallback callback) {
    if (!callback) {
      return {};
    }

    {
      std::lock_guard<std::mutex> lock{mutex_};
      if (!cancelled_) {
        const std::uint64_t id = next_callback_id_++;
        callbacks_.emplace(id, std::move(callback));
        return CancellationRegistration{shared_from_this(), id};
      }
    }

    InvokeSafely(callback);
    return {};
  }

  bool Unregister(std::uint64_t id) noexcept {
    if (!IsValidRegistrationId(id)) {
      return false;
    }

    std::lock_guard<std::mutex> lock{mutex_};
    return callbacks_.erase(id) > 0U;
  }

  [[nodiscard]] bool ContainsRegistration(std::uint64_t id) const noexcept {
    if (!IsValidRegistrationId(id)) {
      return false;
    }

    std::lock_guard<std::mutex> lock{mutex_};
    return callbacks_.contains(id);
  }

private:
  mutable std::mutex mutex_;
  bool cancelled_{false};
  std::uint64_t next_callback_id_{1U};
  std::map<std::uint64_t, CancellationCallback> callbacks_;
};

} // namespace detail

CancellationRegistration::CancellationRegistration(std::shared_ptr<detail::CancellationState> state,
                                                   std::uint64_t id)
    : state_(std::move(state)), id_(id) {}

CancellationRegistration::~CancellationRegistration() { static_cast<void>(Unregister()); }

CancellationRegistration::CancellationRegistration(CancellationRegistration&& other) noexcept
    : state_(std::move(other.state_)), id_(other.id_) {
  other.id_ = 0U;
}

CancellationRegistration&
CancellationRegistration::operator=(CancellationRegistration&& other) noexcept {
  if (this != &other) {
    static_cast<void>(Unregister());
    state_ = std::move(other.state_);
    id_ = other.id_;
    other.id_ = 0U;
  }
  return *this;
}

bool CancellationRegistration::IsRegistered() const noexcept {
  return state_ != nullptr && state_->ContainsRegistration(id_);
}

bool CancellationRegistration::Unregister() noexcept {
  if (!state_ || !IsValidRegistrationId(id_)) {
    return false;
  }

  const bool removed = state_->Unregister(id_);
  state_.reset();
  id_ = 0U;
  return removed;
}

CancellationToken::CancellationToken(std::shared_ptr<detail::CancellationState> state)
    : state_(std::move(state)) {}

bool CancellationToken::IsCancellationPossible() const noexcept { return state_ != nullptr; }

bool CancellationToken::IsCancelled() const noexcept {
  return state_ != nullptr && state_->IsCancelled();
}

CancellationRegistration CancellationToken::Register(CancellationCallback callback) const {
  if (!state_) {
    return {};
  }
  return state_->Register(std::move(callback));
}

CancellationSource::CancellationSource() { state_ = std::make_shared<detail::CancellationState>(); }

CancellationSource::CancellationSource(const CancellationToken& parent_token)
    : CancellationSource() {
  std::weak_ptr<detail::CancellationState> weak_state{state_};
  parent_registration_ = parent_token.Register([weak_state]() {
    if (const std::shared_ptr<detail::CancellationState> state = weak_state.lock()) {
      static_cast<void>(state->Cancel());
    }
  });
}

bool CancellationSource::Cancel() noexcept { return state_->Cancel(); }

bool CancellationSource::IsCancelled() const noexcept { return state_->IsCancelled(); }

CancellationToken CancellationSource::Token() const noexcept { return CancellationToken{state_}; }

CancellationRegistration CancellationSource::Register(CancellationCallback callback) const {
  return state_->Register(std::move(callback));
}

} // namespace humanoid::runtime
