#pragma once

/**
 * @file SequenceNode.h
 * @brief Defines behavior tree sequence composite nodes.
 */

#include <cstdint>
#include <string>

#include <humanoid/bt/CompositeNode.h>

namespace humanoid::bt {

/**
 * @brief Sequence traversal mode.
 */
enum class SequenceMemoryPolicy : std::uint8_t {
  Stateless, ///< Restart from the first child on every tick.
  Memory     ///< Resume from the running child on the next tick.
};

/**
 * @brief Ticks children in order until one fails or runs.
 *
 * A sequence succeeds only when all children succeed. It fails on the first
 * child failure and aborts on cancellation or child abort.
 */
class SequenceNode final : public CompositeNode {
public:
  /**
   * @brief Constructs a sequence node.
   *
   * @param name Stable diagnostic node name.
   * @param memory_policy Stateless or memory traversal.
   * @param children Initial owned child nodes.
   */
  explicit SequenceNode(std::string name = "Sequence",
                        SequenceMemoryPolicy memory_policy = SequenceMemoryPolicy::Stateless,
                        BTChildren children = {});

  /**
   * @brief Ticks children according to sequence semantics.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Sequence tick status.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

protected:
  void OnInitializeLocked() override;
  void OnResetLocked() override;

private:
  SequenceMemoryPolicy memory_policy_;
  std::size_t current_index_{0U};
};

} // namespace humanoid::bt
