#pragma once

/**
 * @file ResourceManager.h
 * @brief Defines thread-safe runtime resource ownership primitives.
 */

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace humanoid::runtime {

namespace detail {
class ResourceManagerState;
} // namespace detail

/**
 * @brief Resource ownership mode.
 */
enum class ResourceLockMode {
  /** @brief Multiple owners may hold the resource concurrently. */
  Shared,

  /** @brief One owner has exclusive access to the resource. */
  Exclusive
};

/**
 * @brief Immutable identifier for one acquired resource lease.
 *
 * A handle is an opaque value returned by the resource manager. It contains no
 * SDK or robot-specific data and is valid only for the manager state that
 * created it.
 */
class ResourceHandle final {
public:
  /** @brief Constructs an invalid resource handle. */
  ResourceHandle() = default;

  /**
   * @brief Returns the logical resource identifier.
   *
   * @return Resource identifier, or an empty string for an invalid handle.
   */
  [[nodiscard]] std::string_view ResourceId() const noexcept;

  /**
   * @brief Returns the lock mode for this lease.
   *
   * @return Shared or exclusive lock mode.
   */
  [[nodiscard]] ResourceLockMode Mode() const noexcept;

  /**
   * @brief Reports whether the handle refers to a manager-created lease.
   *
   * @return True when the handle contains a non-empty resource and lease id.
   */
  [[nodiscard]] bool IsValid() const noexcept;

private:
  friend class detail::ResourceManagerState;
  friend class ResourceManager;
  friend class ResourceLock;

  ResourceHandle(std::string resource_id, ResourceLockMode mode, std::uint64_t lease_id);

  std::string resource_id_;
  ResourceLockMode mode_{ResourceLockMode::Shared};
  std::uint64_t lease_id_{0U};
};

/**
 * @brief Move-only RAII guard for a runtime resource lease.
 *
 * Destroying a valid lock releases the associated resource lease. `Release()`
 * may be called explicitly when deterministic unlock timing is required.
 */
class ResourceLock final {
public:
  /** @brief Constructs an empty lock that owns no resource. */
  ResourceLock() = default;

  /** @brief Releases an owned resource lease. */
  ~ResourceLock();

  ResourceLock(const ResourceLock&) = delete;
  ResourceLock& operator=(const ResourceLock&) = delete;

  /** @brief Moves ownership from another lock. */
  ResourceLock(ResourceLock&& other) noexcept;

  /** @brief Releases this lock, then moves ownership from another lock. */
  ResourceLock& operator=(ResourceLock&& other) noexcept;

  /**
   * @brief Reports whether this lock currently owns a resource lease.
   *
   * @return True while this RAII object owns an active lease.
   */
  [[nodiscard]] bool OwnsLock() const noexcept;

  /**
   * @brief Returns the immutable resource handle associated with this lock.
   *
   * @return Resource handle. The handle may be invalid when the lock is empty.
   */
  [[nodiscard]] const ResourceHandle& Handle() const noexcept;

  /**
   * @brief Releases the owned resource lease if present.
   *
   * @return True when this call released an active lease.
   */
  bool Release() noexcept;

private:
  friend class detail::ResourceManagerState;
  friend class ResourceManager;

  ResourceLock(std::shared_ptr<detail::ResourceManagerState> state, ResourceHandle handle);

  std::shared_ptr<detail::ResourceManagerState> state_;
  ResourceHandle handle_;
  bool owns_lock_{false};
};

/**
 * @brief Thread-safe manager for shared and exclusive runtime resources.
 *
 * The manager prevents independent runtime instances in the same process from
 * owning incompatible leases for a logical robot resource. It performs no SDK,
 * robot, mission, or behavior-tree work.
 */
class ResourceManager final {
public:
  /** @brief Constructs an empty resource manager. */
  ResourceManager();

  /** @brief Destroys the manager state after all remaining shared leases end. */
  ~ResourceManager();

  ResourceManager(const ResourceManager&) = delete;
  ResourceManager& operator=(const ResourceManager&) = delete;
  ResourceManager(ResourceManager&&) = delete;
  ResourceManager& operator=(ResourceManager&&) = delete;

  /**
   * @brief Blocks until a resource lease is acquired.
   *
   * Empty resource identifiers are rejected and return an empty lock.
   *
   * @param resource_id Logical resource identifier.
   * @param mode Requested shared or exclusive ownership mode.
   * @return RAII lock owning the lease, or an empty lock for invalid input.
   */
  [[nodiscard]] ResourceLock Acquire(std::string_view resource_id, ResourceLockMode mode);

  /**
   * @brief Attempts to acquire a resource lease without blocking.
   *
   * @param resource_id Logical resource identifier.
   * @param mode Requested shared or exclusive ownership mode.
   * @return RAII lock when acquisition succeeds; empty when unavailable or invalid.
   */
  [[nodiscard]] std::optional<ResourceLock> TryAcquire(std::string_view resource_id,
                                                       ResourceLockMode mode);

  /**
   * @brief Attempts to acquire a resource lease before a timeout expires.
   *
   * @param resource_id Logical resource identifier.
   * @param mode Requested shared or exclusive ownership mode.
   * @param timeout Maximum time to wait.
   * @return RAII lock when acquisition succeeds; empty on timeout or invalid input.
   */
  [[nodiscard]] std::optional<ResourceLock>
  Acquire(std::string_view resource_id, ResourceLockMode mode, std::chrono::milliseconds timeout);

  /**
   * @brief Releases a previously acquired resource lease.
   *
   * RAII locks normally call this automatically. The function is provided for
   * integration points that need to release through a copied handle.
   *
   * @param handle Resource handle returned by this manager.
   * @return True when an active lease was released.
   */
  bool Release(const ResourceHandle& handle) noexcept;

  /**
   * @brief Returns the number of active leases for a resource.
   *
   * @param resource_id Logical resource identifier.
   * @return Active shared leases plus an active exclusive lease.
   */
  [[nodiscard]] std::size_t ActiveLockCount(std::string_view resource_id) const;

private:
  std::shared_ptr<detail::ResourceManagerState> state_;
};

} // namespace humanoid::runtime
