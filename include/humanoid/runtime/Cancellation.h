#pragma once

/**
 * @file Cancellation.h
 * @brief Defines framework-wide cooperative cancellation primitives.
 */

#include <cstdint>
#include <functional>
#include <memory>

namespace humanoid::runtime {

namespace detail {
class CancellationState;
} // namespace detail

/**
 * @brief Callback invoked when cooperative cancellation is requested.
 */
using CancellationCallback = std::function<void()>;

class CancellationToken;

/**
 * @brief Move-only RAII registration for a cancellation callback.
 *
 * Destroying the registration unregisters the callback when cancellation has
 * not started yet. If cancellation is already in progress, the callback may
 * already have been selected for invocation and will run outside internal
 * locks.
 */
class CancellationRegistration final {
public:
  /** @brief Constructs an empty registration. */
  CancellationRegistration() = default;

  /** @brief Unregisters the callback if it is still pending. */
  ~CancellationRegistration();

  CancellationRegistration(const CancellationRegistration&) = delete;
  CancellationRegistration& operator=(const CancellationRegistration&) = delete;

  /** @brief Moves a callback registration. */
  CancellationRegistration(CancellationRegistration&& other) noexcept;

  /** @brief Unregisters this callback, then moves another registration. */
  CancellationRegistration& operator=(CancellationRegistration&& other) noexcept;

  /**
   * @brief Reports whether this object currently owns a pending registration.
   *
   * @return True when the callback can still be unregistered.
   */
  [[nodiscard]] bool IsRegistered() const noexcept;

  /**
   * @brief Unregisters the callback when cancellation has not started.
   *
   * @return True when a pending callback was removed.
   */
  bool Unregister() noexcept;

private:
  friend class CancellationToken;
  friend class detail::CancellationState;

  CancellationRegistration(std::shared_ptr<detail::CancellationState> state, std::uint64_t id);

  std::shared_ptr<detail::CancellationState> state_;
  std::uint64_t id_{0U};
};

/**
 * @brief Copyable observation handle for cooperative cancellation state.
 *
 * Tokens can be passed into mission, command, runtime, and future behavior-tree
 * code without transferring cancellation authority.
 */
class CancellationToken final {
public:
  /** @brief Constructs a token with no associated cancellation state. */
  CancellationToken() = default;

  /**
   * @brief Reports whether the token is associated with cancellable state.
   *
   * @return True when the token can observe a source.
   */
  [[nodiscard]] bool IsCancellationPossible() const noexcept;

  /**
   * @brief Reports whether cancellation has been requested.
   *
   * @return True after the associated source is cancelled.
   */
  [[nodiscard]] bool IsCancelled() const noexcept;

  /**
   * @brief Registers a callback for cancellation.
   *
   * If cancellation was already requested, the callback is invoked before this
   * function returns and an empty registration is returned.
   *
   * @param callback Callback to invoke on cancellation.
   * @return RAII registration for pending callbacks, or empty when not pending.
   */
  [[nodiscard]] CancellationRegistration Register(CancellationCallback callback) const;

private:
  friend class CancellationSource;

  explicit CancellationToken(std::shared_ptr<detail::CancellationState> state);

  std::shared_ptr<detail::CancellationState> state_;
};

/**
 * @brief Authority object that requests cooperative cancellation.
 *
 * A source owns cancellation state and creates tokens that may be shared with
 * nested execution. Cancellation is monotonic: once requested, it cannot be
 * reset.
 */
class CancellationSource final {
public:
  /** @brief Constructs a new uncancelled source. */
  CancellationSource();

  /**
   * @brief Constructs a source linked to a parent token.
   *
   * Cancelling the parent requests cancellation of this source. Cancelling this
   * source does not cancel the parent.
   *
   * @param parent_token Parent token to observe.
   */
  explicit CancellationSource(const CancellationToken& parent_token);

  /** @brief Destroys the source and its optional parent registration. */
  ~CancellationSource() = default;

  CancellationSource(const CancellationSource&) = delete;
  CancellationSource& operator=(const CancellationSource&) = delete;
  CancellationSource(CancellationSource&&) = delete;
  CancellationSource& operator=(CancellationSource&&) = delete;

  /**
   * @brief Requests cooperative cancellation.
   *
   * Registered callbacks are invoked outside internal locks. Exceptions thrown
   * by callbacks are contained so cancellation remains reliable.
   *
   * @return True only for the first cancellation request.
   */
  bool Cancel() noexcept;

  /**
   * @brief Reports whether cancellation has been requested.
   *
   * @return True after `Cancel()` succeeds or a linked parent is cancelled.
   */
  [[nodiscard]] bool IsCancelled() const noexcept;

  /**
   * @brief Returns a token sharing this source's cancellation state.
   *
   * @return Copyable cancellation token.
   */
  [[nodiscard]] CancellationToken Token() const noexcept;

  /**
   * @brief Registers a callback for cancellation.
   *
   * @param callback Callback to invoke on cancellation.
   * @return RAII registration for pending callbacks, or empty when not pending.
   */
  [[nodiscard]] CancellationRegistration Register(CancellationCallback callback) const;

private:
  std::shared_ptr<detail::CancellationState> state_;
  CancellationRegistration parent_registration_;
};

} // namespace humanoid::runtime
