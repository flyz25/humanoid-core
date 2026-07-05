#pragma once

/**
 * @file DecoratorNode.h
 * @brief Defines shared behavior tree decorator node ownership.
 */

#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BTStatus.h>

namespace humanoid::bt {

/**
 * @brief Base class for thread-safe behavior tree decorator nodes.
 *
 * A decorator owns one optional child node and modifies that child's status or
 * execution policy. It contains no robot, mission, parser, plugin, or SDK
 * logic. Derived decorators define only tick policy.
 */
class DecoratorNode : public BTNode {
public:
  /**
   * @brief Constructs a decorator node.
   *
   * @param name Stable diagnostic node name.
   * @param child Initial owned child node.
   */
  explicit DecoratorNode(std::string name, std::unique_ptr<BTNode> child = {});

  /** @brief Destroys the decorator and owned child node. */
  ~DecoratorNode() noexcept override = default;

  DecoratorNode(const DecoratorNode&) = delete;
  DecoratorNode& operator=(const DecoratorNode&) = delete;
  DecoratorNode(DecoratorNode&&) = delete;
  DecoratorNode& operator=(DecoratorNode&&) = delete;

  /**
   * @brief Returns the stable decorator node name.
   *
   * @return Non-owning node name.
   */
  [[nodiscard]] std::string_view Name() const noexcept override;

  /**
   * @brief Initializes the child node.
   *
   * @param context Runtime-backed behavior tree context.
   * @return Child initialization status, or `Failure` when no child exists.
   */
  [[nodiscard]] BTStatus Initialize(BTContext& context) override;

  /**
   * @brief Resets decorator and child state.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Reset(BTContext& context) override;

  /**
   * @brief Shuts down the child node.
   *
   * Child shutdown exceptions are contained.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Shutdown(BTContext& context) override;

  /**
   * @brief Replaces the owned child node.
   *
   * @param child New owned child node.
   * @return True when a non-null child was accepted.
   */
  bool SetChild(std::unique_ptr<BTNode> child);

  /**
   * @brief Reports whether the decorator currently owns a child.
   *
   * @return True when a child node exists.
   */
  [[nodiscard]] bool HasChild() const;

protected:
  /**
   * @brief Called while the decorator lock is held during initialization.
   */
  virtual void OnInitializeLocked();

  /**
   * @brief Called while the decorator lock is held during reset.
   */
  virtual void OnResetLocked();

  mutable std::mutex mutex_;
  std::unique_ptr<BTNode> child_;

private:
  std::string name_;
};

} // namespace humanoid::bt
