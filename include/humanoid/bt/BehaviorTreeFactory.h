#pragma once

/**
 * @file BehaviorTreeFactory.h
 * @brief Defines behavior tree node registration and tree creation utilities.
 */

#include <functional>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

#include <humanoid/bt/BTContext.h>
#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BehaviorTree.h>

namespace humanoid::bt {

/**
 * @brief Factory callback that creates one behavior tree node.
 */
using BTNodeCreator = std::function<std::unique_ptr<BTNode>()>;

/**
 * @brief Thread-safe registry for behavior tree node creator functions.
 *
 * The factory owns creator callbacks only. It performs no XML parsing, mission
 * loading, robot adapter construction, SDK communication, or business logic.
 */
class BehaviorTreeFactory final {
public:
  /** @brief Constructs an empty factory. */
  BehaviorTreeFactory() = default;

  /** @brief Destroys registered creator callbacks. */
  ~BehaviorTreeFactory() = default;

  BehaviorTreeFactory(const BehaviorTreeFactory&) = delete;
  BehaviorTreeFactory& operator=(const BehaviorTreeFactory&) = delete;
  BehaviorTreeFactory(BehaviorTreeFactory&&) = delete;
  BehaviorTreeFactory& operator=(BehaviorTreeFactory&&) = delete;

  /**
   * @brief Registers a node creator under a stable type name.
   *
   * Empty type names and empty callbacks are rejected. Existing type names are
   * not replaced.
   *
   * @param node_type Stable node type name.
   * @param creator Creator callback.
   * @return True when registration succeeded.
   */
  bool RegisterNode(std::string node_type, BTNodeCreator creator);

  /**
   * @brief Removes one node creator.
   *
   * @param node_type Node type to unregister.
   * @return True when a creator was removed.
   */
  bool UnregisterNode(std::string_view node_type);

  /**
   * @brief Reports whether a node type is registered.
   *
   * @param node_type Node type to inspect.
   * @return True when registered.
   */
  [[nodiscard]] bool Contains(std::string_view node_type) const;

  /**
   * @brief Creates one node by registered type.
   *
   * Creator exceptions are contained and reported as an empty result.
   *
   * @param node_type Registered node type.
   * @return Owned node, or empty when creation fails.
   */
  [[nodiscard]] std::unique_ptr<BTNode> CreateNode(std::string_view node_type) const;

  /**
   * @brief Creates a behavior tree whose root is a registered node type.
   *
   * @param root_node_type Registered root node type.
   * @param context Runtime-backed context to pass to the tree.
   * @return Owned tree, or empty when root creation fails.
   */
  [[nodiscard]] std::unique_ptr<BehaviorTree> CreateTree(std::string_view root_node_type,
                                                         BTContext context = {}) const;

  /**
   * @brief Enumerates registered node type names.
   *
   * @return Sorted node type names.
   */
  [[nodiscard]] std::vector<std::string> RegisteredNodeTypes() const;

  /**
   * @brief Returns the number of registered node creators.
   *
   * @return Creator count.
   */
  [[nodiscard]] std::size_t Size() const;

private:
  mutable std::shared_mutex mutex_;
  std::map<std::string, BTNodeCreator, std::less<>> creators_;
};

} // namespace humanoid::bt
