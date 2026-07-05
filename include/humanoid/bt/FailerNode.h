#pragma once

/**
 * @file FailerNode.h
 * @brief Defines the behavior tree failer decorator.
 */

#include <memory>
#include <string>

#include <humanoid/bt/DecoratorNode.h>

namespace humanoid::bt {

/**
 * @brief Converts child terminal success or failure into failure.
 */
class FailerNode final : public DecoratorNode {
public:
  /**
   * @brief Constructs a failer decorator.
   *
   * @param name Stable diagnostic node name.
   * @param child Initial owned child node.
   */
  explicit FailerNode(std::string name = "Failer", std::unique_ptr<BTNode> child = {});

  /**
   * @brief Ticks the child and converts terminal success/failure to failure.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Decorated child status.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;
};

} // namespace humanoid::bt
