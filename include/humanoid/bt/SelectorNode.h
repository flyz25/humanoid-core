#pragma once

/**
 * @file SelectorNode.h
 * @brief Defines behavior tree selector composite nodes.
 */

#include <cstdint>
#include <string>

#include <humanoid/bt/CompositeNode.h>

namespace humanoid::bt {

/**
 * @brief Selector traversal mode.
 */
enum class SelectorMemoryPolicy : std::uint8_t {
  Stateless, ///< Restart from the first child on every tick.
  Memory     ///< Resume from the running child on the next tick.
};

/**
 * @brief Ticks children in order until one succeeds or runs.
 *
 * A selector succeeds on the first child success. It fails only when all
 * children fail and aborts on cancellation or child abort.
 */
class SelectorNode final : public CompositeNode {
public:
  /**
   * @brief Constructs a selector node.
   *
   * @param name Stable diagnostic node name.
   * @param memory_policy Stateless or memory traversal.
   * @param children Initial owned child nodes.
   */
  explicit SelectorNode(std::string name = "Selector",
                        SelectorMemoryPolicy memory_policy = SelectorMemoryPolicy::Stateless,
                        BTChildren children = {});

  /**
   * @brief Ticks children according to selector semantics.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Selector tick status.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

protected:
  void OnInitializeLocked() override;
  void OnResetLocked() override;

private:
  SelectorMemoryPolicy memory_policy_;
  std::size_t current_index_{0U};
};

} // namespace humanoid::bt
