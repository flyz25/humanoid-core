#pragma once

/**
 * @file CompositeNode.h
 * @brief Defines shared behavior tree composite node ownership.
 */

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BTStatus.h>

namespace humanoid::bt {

/**
 * @brief Collection of owned behavior tree child nodes.
 */
using BTChildren = std::vector<std::unique_ptr<BTNode>>;

/**
 * @brief Base class for thread-safe behavior tree composite nodes.
 *
 * `CompositeNode` owns child nodes and provides common lifecycle propagation.
 * It contains no robot, mission, parser, plugin, or SDK logic. Derived
 * composites define only child tick policy.
 */
class CompositeNode : public BTNode {
public:
  /**
   * @brief Constructs a composite node.
   *
   * Null children are discarded so the composite always owns valid nodes.
   *
   * @param name Stable diagnostic node name.
   * @param children Initial owned child nodes.
   */
  explicit CompositeNode(std::string name, BTChildren children = {});

  /** @brief Shuts down and destroys owned child nodes. */
  ~CompositeNode() noexcept override = default;

  CompositeNode(const CompositeNode&) = delete;
  CompositeNode& operator=(const CompositeNode&) = delete;
  CompositeNode(CompositeNode&&) = delete;
  CompositeNode& operator=(CompositeNode&&) = delete;

  /**
   * @brief Returns the stable composite node name.
   *
   * @return Non-owning node name.
   */
  [[nodiscard]] std::string_view Name() const noexcept override;

  /**
   * @brief Initializes all child nodes.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Failure` or `Aborted` if any child reports that status,
   * otherwise `Idle`.
   */
  [[nodiscard]] BTStatus Initialize(BTContext& context) override;

  /**
   * @brief Resets all child nodes and composite traversal state.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Reset(BTContext& context) override;

  /**
   * @brief Shuts down all child nodes.
   *
   * Child shutdown exceptions are contained so all children receive shutdown.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Shutdown(BTContext& context) override;

  /**
   * @brief Adds one child node.
   *
   * @param child Owned child node.
   * @return True when the child was accepted.
   */
  bool AddChild(std::unique_ptr<BTNode> child);

  /**
   * @brief Returns the current number of children.
   *
   * @return Child count.
   */
  [[nodiscard]] std::size_t ChildCount() const;

protected:
  /**
   * @brief Called while the composite lock is held during initialization.
   */
  virtual void OnInitializeLocked();

  /**
   * @brief Called while the composite lock is held during reset.
   */
  virtual void OnResetLocked();

  mutable std::mutex mutex_;
  BTChildren children_;

private:
  std::string name_;
};

} // namespace humanoid::bt
