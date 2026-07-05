#pragma once

/**
 * @file DelayNode.h
 * @brief Defines a behavior tree timed delay leaf node.
 */

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BTStatus.h>

namespace humanoid::bt {

/**
 * @brief Leaf node that returns `Running` until a steady-clock duration elapses.
 */
class DelayNode final : public BTNode {
public:
  /**
   * @brief Constructs a delay node.
   *
   * @param name Stable diagnostic node name.
   * @param duration Delay duration. Non-positive durations complete
   * immediately.
   */
  explicit DelayNode(std::string name = "Delay",
                     std::chrono::nanoseconds duration = std::chrono::nanoseconds{0});

  /** @brief Destroys the node. */
  ~DelayNode() noexcept override = default;

  DelayNode(const DelayNode&) = delete;
  DelayNode& operator=(const DelayNode&) = delete;
  DelayNode(DelayNode&&) = delete;
  DelayNode& operator=(DelayNode&&) = delete;

  /**
   * @brief Returns the stable delay node name.
   *
   * @return Non-owning node name.
   */
  [[nodiscard]] std::string_view Name() const noexcept override;

  /**
   * @brief Clears previous timing state.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Idle`.
   */
  [[nodiscard]] BTStatus Initialize(BTContext& context) override;

  /**
   * @brief Advances the delay without busy waiting.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Running` until elapsed, `Success` when complete, or `Aborted` on
   * cancellation.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

  /**
   * @brief Clears timing state.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Reset(BTContext& context) override;

  /**
   * @brief Releases delay resources.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Shutdown(BTContext& context) override;

private:
  /**
   * @brief Clears active timing state and completion state.
   */
  void ClearTimer() noexcept;

  mutable std::mutex mutex_;
  std::string name_;
  std::chrono::nanoseconds duration_;
  std::optional<std::chrono::steady_clock::time_point> start_time_;
  bool completed_{false};
};

} // namespace humanoid::bt
