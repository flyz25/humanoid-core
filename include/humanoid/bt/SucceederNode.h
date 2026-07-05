#pragma once

/**
 * @file SucceederNode.h
 * @brief Defines the behavior tree succeeder decorator.
 */

#include <memory>
#include <string>

#include <humanoid/bt/DecoratorNode.h>

namespace humanoid::bt {

/**
 * @brief Converts child terminal success or failure into success.
 */
class SucceederNode final : public DecoratorNode {
public:
  /**
   * @brief Constructs a succeeder decorator.
   *
   * @param name Stable diagnostic node name.
   * @param child Initial owned child node.
   */
  explicit SucceederNode(std::string name = "Succeeder", std::unique_ptr<BTNode> child = {});

  /**
   * @brief Ticks the child and converts terminal success/failure to success.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Decorated child status.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;
};

} // namespace humanoid::bt
