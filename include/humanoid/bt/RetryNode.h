#pragma once

/**
 * @file RetryNode.h
 * @brief Defines the behavior tree retry decorator.
 */

#include <cstddef>
#include <memory>
#include <string>

#include <humanoid/bt/DecoratorNode.h>

namespace humanoid::bt {

/**
 * @brief Retries a failing child until it succeeds or attempts are exhausted.
 */
class RetryNode final : public DecoratorNode {
public:
  /**
   * @brief Constructs a retry decorator.
   *
   * @param name Stable diagnostic node name.
   * @param max_attempts Maximum failed attempts before returning failure.
   * Values below one are normalized to one.
   * @param child Initial owned child node.
   */
  explicit RetryNode(std::string name = "Retry", std::size_t max_attempts = 1U,
                     std::unique_ptr<BTNode> child = {});

  /**
   * @brief Ticks and retries the child on failure.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Running` while another retry remains, otherwise child status.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

protected:
  /**
   * @brief Resets failed-attempt count when the node is initialized.
   */
  void OnInitializeLocked() override;

  /**
   * @brief Resets failed-attempt count when the node is reset.
   */
  void OnResetLocked() override;

private:
  std::size_t max_attempts_;
  std::size_t failed_attempts_{0U};
};

} // namespace humanoid::bt
