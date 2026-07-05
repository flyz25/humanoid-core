#pragma once

/**
 * @file ParallelNode.h
 * @brief Defines behavior tree parallel composite nodes.
 */

#include <cstddef>
#include <string>

#include <humanoid/bt/CompositeNode.h>

namespace humanoid::bt {

/**
 * @brief Ticks all children concurrently and aggregates their statuses.
 *
 * By default a parallel node succeeds when all children succeed and fails when
 * any child fails. Thresholds can be configured for alternative aggregation
 * policies. A threshold value of zero for success means all children.
 */
class ParallelNode final : public CompositeNode {
public:
  /**
   * @brief Constructs a parallel node.
   *
   * @param name Stable diagnostic node name.
   * @param success_threshold Number of successful children required; zero
   * means all children.
   * @param failure_threshold Number of failed children required to fail.
   * @param children Initial owned child nodes.
   */
  explicit ParallelNode(std::string name = "Parallel", std::size_t success_threshold = 0U,
                        std::size_t failure_threshold = 1U, BTChildren children = {});

  /**
   * @brief Ticks all child nodes concurrently.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Aggregated parallel status.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

private:
  std::size_t success_threshold_;
  std::size_t failure_threshold_;
};

} // namespace humanoid::bt
