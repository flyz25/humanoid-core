#include <humanoid/runtime/ResourceManager.h>

#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
#include <utility>

namespace humanoid::runtime {
namespace {

[[nodiscard]] bool IsValidResourceId(std::string_view resource_id) noexcept {
  return !resource_id.empty();
}

struct ResourceState final {
  std::set<std::uint64_t> shared_leases;
  std::optional<std::uint64_t> exclusive_lease;
  std::size_t waiting_shared{0U};
  std::size_t waiting_exclusive{0U};
};

[[nodiscard]] bool CanAcquireShared(const ResourceState& resource) noexcept {
  return !resource.exclusive_lease.has_value() && resource.waiting_exclusive == 0U;
}

[[nodiscard]] bool CanAcquireExclusive(const ResourceState& resource) noexcept {
  return !resource.exclusive_lease.has_value() && resource.shared_leases.empty();
}

[[nodiscard]] bool CanAcquire(const ResourceState& resource, ResourceLockMode mode) noexcept {
  return mode == ResourceLockMode::Shared ? CanAcquireShared(resource)
                                          : CanAcquireExclusive(resource);
}

} // namespace

namespace detail {

class ResourceManagerState final : public std::enable_shared_from_this<ResourceManagerState> {
public:
  [[nodiscard]] ResourceLock Acquire(std::string_view resource_id, ResourceLockMode mode) {
    if (!IsValidResourceId(resource_id)) {
      return {};
    }

    std::unique_lock<std::mutex> lock{mutex_};
    ResourceState& resource = resources_[std::string{resource_id}];
    if (mode == ResourceLockMode::Exclusive) {
      ++resource.waiting_exclusive;
      condition_.wait(lock, [&resource]() { return CanAcquireExclusive(resource); });
      --resource.waiting_exclusive;
    } else {
      ++resource.waiting_shared;
      condition_.wait(lock, [&resource]() { return CanAcquireShared(resource); });
      --resource.waiting_shared;
    }
    return CreateLockLocked(resource_id, mode, resource);
  }

  [[nodiscard]] std::optional<ResourceLock> TryAcquire(std::string_view resource_id,
                                                       ResourceLockMode mode) {
    if (!IsValidResourceId(resource_id)) {
      return std::nullopt;
    }

    std::unique_lock<std::mutex> lock{mutex_};
    ResourceState& resource = resources_[std::string{resource_id}];
    if (!CanAcquire(resource, mode)) {
      RemoveUnusedResourceLocked(resource_id, resource);
      return std::nullopt;
    }
    return CreateLockLocked(resource_id, mode, resource);
  }

  [[nodiscard]] std::optional<ResourceLock>
  Acquire(std::string_view resource_id, ResourceLockMode mode, std::chrono::milliseconds timeout) {
    if (!IsValidResourceId(resource_id)) {
      return std::nullopt;
    }

    if (timeout <= std::chrono::milliseconds::zero()) {
      return TryAcquire(resource_id, mode);
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::unique_lock<std::mutex> lock{mutex_};
    ResourceState& resource = resources_[std::string{resource_id}];

    if (mode == ResourceLockMode::Exclusive) {
      ++resource.waiting_exclusive;
      const bool acquired = condition_.wait_until(
          lock, deadline, [&resource]() { return CanAcquireExclusive(resource); });
      --resource.waiting_exclusive;
      if (!acquired) {
        RemoveUnusedResourceLocked(resource_id, resource);
        condition_.notify_all();
        return std::nullopt;
      }
    } else {
      ++resource.waiting_shared;
      const bool acquired = condition_.wait_until(
          lock, deadline, [&resource]() { return CanAcquireShared(resource); });
      --resource.waiting_shared;
      if (!acquired) {
        RemoveUnusedResourceLocked(resource_id, resource);
        return std::nullopt;
      }
    }

    return CreateLockLocked(resource_id, mode, resource);
  }

  bool Release(const ResourceHandle& handle) noexcept {
    if (!handle.IsValid()) {
      return false;
    }

    std::unique_lock<std::mutex> lock{mutex_};
    const auto resource_iterator = resources_.find(handle.resource_id_);
    if (resource_iterator == resources_.end()) {
      return false;
    }

    ResourceState& resource = resource_iterator->second;
    bool released = false;
    if (handle.mode_ == ResourceLockMode::Exclusive) {
      if (resource.exclusive_lease == handle.lease_id_) {
        resource.exclusive_lease.reset();
        released = true;
      }
    } else {
      released = resource.shared_leases.erase(handle.lease_id_) > 0U;
    }

    if (!released) {
      return false;
    }

    if (resource.shared_leases.empty() && !resource.exclusive_lease.has_value() &&
        resource.waiting_shared == 0U && resource.waiting_exclusive == 0U) {
      resources_.erase(resource_iterator);
    }

    lock.unlock();
    condition_.notify_all();
    return true;
  }

  [[nodiscard]] std::size_t ActiveLockCount(std::string_view resource_id) const {
    if (!IsValidResourceId(resource_id)) {
      return 0U;
    }

    std::lock_guard<std::mutex> lock{mutex_};
    const auto resource_iterator = resources_.find(resource_id);
    if (resource_iterator == resources_.end()) {
      return 0U;
    }

    const ResourceState& resource = resource_iterator->second;
    return resource.shared_leases.size() + (resource.exclusive_lease.has_value() ? 1U : 0U);
  }

private:
  [[nodiscard]] ResourceLock CreateLockLocked(std::string_view resource_id, ResourceLockMode mode,
                                              ResourceState& resource) {
    const std::uint64_t lease_id = next_lease_id_++;
    ResourceHandle handle{std::string{resource_id}, mode, lease_id};
    std::shared_ptr<ResourceManagerState> state = shared_from_this();
    if (mode == ResourceLockMode::Exclusive) {
      resource.exclusive_lease = lease_id;
    } else {
      resource.shared_leases.insert(lease_id);
    }
    return ResourceLock{std::move(state), std::move(handle)};
  }

  void RemoveUnusedResourceLocked(std::string_view resource_id, const ResourceState& resource) {
    if (resource.shared_leases.empty() && !resource.exclusive_lease.has_value() &&
        resource.waiting_shared == 0U && resource.waiting_exclusive == 0U) {
      const auto resource_iterator = resources_.find(resource_id);
      if (resource_iterator != resources_.end()) {
        resources_.erase(resource_iterator);
      }
    }
  }

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::map<std::string, ResourceState, std::less<>> resources_;
  std::uint64_t next_lease_id_{1U};
};

} // namespace detail

ResourceHandle::ResourceHandle(std::string resource_id, ResourceLockMode mode,
                               std::uint64_t lease_id)
    : resource_id_(std::move(resource_id)), mode_(mode), lease_id_(lease_id) {}

std::string_view ResourceHandle::ResourceId() const noexcept { return resource_id_; }

ResourceLockMode ResourceHandle::Mode() const noexcept { return mode_; }

bool ResourceHandle::IsValid() const noexcept { return !resource_id_.empty() && lease_id_ != 0U; }

ResourceLock::ResourceLock(std::shared_ptr<detail::ResourceManagerState> state,
                           ResourceHandle handle)
    : state_(std::move(state)), handle_(std::move(handle)), owns_lock_(handle_.IsValid()) {}

ResourceLock::~ResourceLock() { static_cast<void>(Release()); }

ResourceLock::ResourceLock(ResourceLock&& other) noexcept
    : state_(std::move(other.state_)), handle_(std::move(other.handle_)),
      owns_lock_(other.owns_lock_) {
  other.owns_lock_ = false;
}

ResourceLock& ResourceLock::operator=(ResourceLock&& other) noexcept {
  if (this != &other) {
    static_cast<void>(Release());
    state_ = std::move(other.state_);
    handle_ = std::move(other.handle_);
    owns_lock_ = other.owns_lock_;
    other.owns_lock_ = false;
  }
  return *this;
}

bool ResourceLock::OwnsLock() const noexcept { return owns_lock_; }

const ResourceHandle& ResourceLock::Handle() const noexcept { return handle_; }

bool ResourceLock::Release() noexcept {
  if (!owns_lock_ || !state_) {
    return false;
  }

  const bool released = state_->Release(handle_);
  owns_lock_ = false;
  return released;
}

ResourceManager::ResourceManager() { state_ = std::make_shared<detail::ResourceManagerState>(); }

ResourceManager::~ResourceManager() = default;

ResourceLock ResourceManager::Acquire(std::string_view resource_id, ResourceLockMode mode) {
  return state_->Acquire(resource_id, mode);
}

std::optional<ResourceLock> ResourceManager::TryAcquire(std::string_view resource_id,
                                                        ResourceLockMode mode) {
  return state_->TryAcquire(resource_id, mode);
}

std::optional<ResourceLock> ResourceManager::Acquire(std::string_view resource_id,
                                                     ResourceLockMode mode,
                                                     std::chrono::milliseconds timeout) {
  return state_->Acquire(resource_id, mode, timeout);
}

bool ResourceManager::Release(const ResourceHandle& handle) noexcept {
  return state_->Release(handle);
}

std::size_t ResourceManager::ActiveLockCount(std::string_view resource_id) const {
  return state_->ActiveLockCount(resource_id);
}

} // namespace humanoid::runtime
