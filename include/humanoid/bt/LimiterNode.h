#pragma once

/**
 * @file LimiterNode.h
 * @brief Defines the behavior tree limiter decorator.
 */

#include <cstddef>
#include <memory>
#include <string>

#include <humanoid/bt/DecoratorNode.h>

namespace humanoid::bt {

/**
 * @brief Limits how many times a child can be ticked before reset.
 */
class LimiterNode final : public DecoratorNode {
public:
  /**
   * @brief Constructs a limiter decorator.
   *
   * @param name Stable diagnostic node name.
   * @param max_ticks Maximum child ticks before returning failure.
   * @param child Initial owned child node.
   */
  explicit LimiterNode(std::string name = "Limiter", std::size_t max_ticks = 1U,
                       std::unique_ptr<BTNode> child = {});

  /**
   * @brief Ticks the child while capacity remains.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Child status while under limit, otherwise `Failure`.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

protected:
  /**
   * @brief Resets consumed tick count when the node is initialized.
   */
  void OnInitializeLocked() override;

  /**
   * @brief Resets consumed tick count when the node is reset.
   */
  void OnResetLocked() override;

private:
  std::size_t max_ticks_;
  std::size_t tick_count_{0U};
};

} // namespace humanoid::bt
