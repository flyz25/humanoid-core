#pragma once

/**
 * @file ScopeExit.hpp
 * @brief Defines a small RAII scope-exit utility.
 */

#include <type_traits>
#include <utility>

namespace humanoid::utilities {

/**
 * @brief Runs a callable when leaving scope unless released.
 *
 * @tparam Callable Callable type with no required arguments.
 */
template <typename Callable> class ScopeExit final {
public:
  /**
   * @brief Constructs a scope guard from a callable.
   *
   * @param callable Callable to run at scope exit.
   */
  explicit ScopeExit(Callable callable) noexcept(
      std::is_nothrow_move_constructible<Callable>::value)
      : callable_(std::move(callable)) {}

  /**
   * @brief Moves a scope guard and transfers ownership of the exit action.
   *
   * @param other Guard to move from.
   */
  ScopeExit(ScopeExit&& other) noexcept(std::is_nothrow_move_constructible<Callable>::value)
      : callable_(std::move(other.callable_)), active_(other.active_) {
    other.release();
  }

  /**
   * @brief Copy construction is disabled because the exit action has single ownership.
   */
  ScopeExit(const ScopeExit&) = delete;

  /**
   * @brief Copy assignment is disabled because the exit action has single ownership.
   *
   * @return This guard.
   */
  ScopeExit& operator=(const ScopeExit&) = delete;

  /**
   * @brief Move assignment is disabled to keep ownership transfer explicit at construction.
   *
   * @return This guard.
   */
  ScopeExit& operator=(ScopeExit&&) = delete;

  /**
   * @brief Runs the callable if the guard is still active.
   */
  ~ScopeExit() noexcept(noexcept(std::declval<Callable&>()())) {
    if (active_) {
      callable_();
    }
  }

  /**
   * @brief Prevents the callable from running at scope exit.
   */
  void release() noexcept { active_ = false; }

  /**
   * @brief Reports whether the guard will run its callable.
   *
   * @return True when the guard is active.
   */
  [[nodiscard]] bool active() const noexcept { return active_; }

private:
  Callable callable_;
  bool active_{true};
};

/**
 * @brief Creates a scope-exit guard with type deduction.
 *
 * @tparam Callable Callable type with no required arguments.
 * @param callable Callable to run at scope exit.
 * @return Scope-exit guard owning the callable.
 */
template <typename Callable> [[nodiscard]] ScopeExit<Callable> makeScopeExit(Callable callable) {
  return ScopeExit<Callable>{std::move(callable)};
}

} // namespace humanoid::utilities
