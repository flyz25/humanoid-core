#pragma once

/**
 * @file TreeValidator.h
 * @brief Defines validation for parsed behavior tree documents.
 */

#include <string>
#include <vector>

namespace humanoid::bt {

class BehaviorTreeFactory;

/**
 * @brief Parsed behavior tree node description.
 */
struct TreeNodeDefinition final {
  /**
   * @brief Registered behavior tree node type name.
   */
  std::string type;

  /**
   * @brief Ordered child node definitions.
   */
  std::vector<TreeNodeDefinition> children;
};

/**
 * @brief Parsed behavior tree document description.
 */
struct TreeDocument final {
  /**
   * @brief Root node definition.
   */
  TreeNodeDefinition root;
};

/**
 * @brief Result returned by behavior tree document validation.
 */
struct TreeValidationResult final {
  /**
   * @brief True when validation succeeded.
   */
  bool success{false};

  /**
   * @brief Human-readable validation diagnostic.
   */
  std::string message;

  /**
   * @brief Reports whether validation succeeded.
   *
   * @return True when the document is valid.
   */
  [[nodiscard]] bool Succeeded() const noexcept { return success; }
};

/**
 * @brief Validates parsed behavior tree documents against registered node types.
 *
 * The validator performs schema and factory-registration checks only. It does
 * not instantiate nodes, parse files, execute trees, call robot adapters, or
 * include vendor SDK headers.
 */
class TreeValidator final {
public:
  /**
   * @brief Constructs a behavior tree validator.
   */
  TreeValidator() = default;

  /**
   * @brief Validates a parsed tree document.
   *
   * @param document Parsed behavior tree document.
   * @param factory Factory containing allowed node registrations.
   * @return Validation result.
   */
  [[nodiscard]] TreeValidationResult Validate(const TreeDocument& document,
                                              const BehaviorTreeFactory& factory) const;
};

} // namespace humanoid::bt
