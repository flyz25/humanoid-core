#pragma once

/**
 * @file TimeoutNode.h
 * @brief Defines the behavior tree timeout decorator.
 */

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include <humanoid/bt/DecoratorNode.h>

namespace humanoid::bt {

/**
 * @brief Fails a running child when a steady-clock timeout expires.
 *
 * The decorator performs one child tick per decorator tick. It cannot preempt a
 * blocking child call; timeout is evaluated before and after the child tick.
 */
class TimeoutNode final : public DecoratorNode {
public:
  /**
   * @brief Constructs a timeout decorator.
   *
   * @param name Stable diagnostic node name.
   * @param timeout Maximum duration before failure. Non-positive durations
   * fail immediately.
   * @param child Initial owned child node.
   */
  explicit TimeoutNode(std::string name = "Timeout",
                       std::chrono::nanoseconds timeout = std::chrono::nanoseconds{0},
                       std::unique_ptr<BTNode> child = {});

  /**
   * @brief Ticks the child while the timeout window remains valid.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Child terminal status, `Running`, or `Failure` on timeout.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

protected:
  /**
   * @brief Clears the active timeout window when the node is initialized.
   */
  void OnInitializeLocked() override;

  /**
   * @brief Clears the active timeout window when the node is reset.
   */
  void OnResetLocked() override;

private:
  /**
   * @brief Checks whether the active timeout window has expired.
   *
   * @param now Steady-clock timestamp to compare against the start time.
   * @return True when the timeout has expired.
   */
  [[nodiscard]] bool TimedOut(std::chrono::steady_clock::time_point now) const noexcept;

  /**
   * @brief Clears the active timeout start timestamp.
   */
  void ClearTimer() noexcept;

  std::chrono::nanoseconds timeout_;
  std::optional<std::chrono::steady_clock::time_point> start_time_;
};

} // namespace humanoid::bt
