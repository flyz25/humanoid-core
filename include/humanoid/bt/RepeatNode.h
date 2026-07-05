#pragma once

/**
 * @file RepeatNode.h
 * @brief Defines the behavior tree repeat decorator.
 */

#include <cstddef>
#include <memory>
#include <string>

#include <humanoid/bt/DecoratorNode.h>

namespace humanoid::bt {

/**
 * @brief Repeats a successful child for a configured number of completions.
 *
 * A repeat count of zero means repeat indefinitely. The decorator performs one
 * child tick per decorator tick and never busy-loops.
 */
class RepeatNode final : public DecoratorNode {
public:
  /**
   * @brief Constructs a repeat decorator.
   *
   * @param name Stable diagnostic node name.
   * @param repeat_count Required successful child completions; zero means
   * repeat forever.
   * @param child Initial owned child node.
   */
  explicit RepeatNode(std::string name = "Repeat", std::size_t repeat_count = 1U,
                      std::unique_ptr<BTNode> child = {});

  /**
   * @brief Ticks and repeats the child.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Running` until repetition is complete, otherwise child failure or
   * abort status.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

protected:
  /**
   * @brief Resets completed repetition count when the node is initialized.
   */
  void OnInitializeLocked() override;

  /**
   * @brief Resets completed repetition count when the node is reset.
   */
  void OnResetLocked() override;

private:
  std::size_t repeat_count_;
  std::size_t completed_count_{0U};
};

} // namespace humanoid::bt
