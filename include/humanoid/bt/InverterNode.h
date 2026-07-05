#pragma once

/**
 * @file InverterNode.h
 * @brief Defines the behavior tree inverter decorator.
 */

#include <memory>
#include <string>

#include <humanoid/bt/DecoratorNode.h>

namespace humanoid::bt {

/**
 * @brief Inverts child success and failure statuses.
 */
class InverterNode final : public DecoratorNode {
public:
  /**
   * @brief Constructs an inverter decorator.
   *
   * @param name Stable diagnostic node name.
   * @param child Initial owned child node.
   */
  explicit InverterNode(std::string name = "Inverter", std::unique_ptr<BTNode> child = {});

  /**
   * @brief Ticks the child and inverts `Success` and `Failure`.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Decorated child status.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;
};

} // namespace humanoid::bt
