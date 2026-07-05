#pragma once

/**
 * @file TreeLoader.h
 * @brief Defines behavior tree loading from JSON and YAML documents.
 */

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <humanoid/bt/BTContext.h>
#include <humanoid/bt/BehaviorTree.h>
#include <humanoid/bt/BehaviorTreeFactory.h>
#include <humanoid/bt/TreeParser.h>
#include <humanoid/bt/TreeValidator.h>

namespace humanoid::bt {

/**
 * @brief Result returned by behavior tree loading.
 */
struct TreeLoadResult final {
  /**
   * @brief Loaded behavior tree. Only meaningful when `success` is true.
   */
  std::unique_ptr<BehaviorTree> tree;

  /**
   * @brief True when loading, parsing, validation, and construction succeeded.
   */
  bool success{false};

  /**
   * @brief Human-readable loading, parsing, validation, or construction diagnostic.
   */
  std::string message;

  /**
   * @brief Reports whether loading succeeded.
   *
   * @return True when a behavior tree was constructed.
   */
  [[nodiscard]] bool Succeeded() const noexcept { return success; }
};

/**
 * @brief Loads behavior tree files and constructs BehaviorTree objects.
 *
 * `TreeLoader` owns parser and validator values through dependency injection
 * and receives registered node creators through `BehaviorTreeFactory`. The
 * behavior tree core types do not know about file formats or parsers.
 */
class TreeLoader final {
public:
  /**
   * @brief Constructs a loader with an injected node factory.
   *
   * @param factory Factory used to construct registered node types.
   */
  explicit TreeLoader(std::shared_ptr<const BehaviorTreeFactory> factory);

  /**
   * @brief Constructs a loader with injected parser, validator, and factory.
   *
   * @param factory Factory used to construct registered node types.
   * @param parser Parser used for text-to-tree conversion.
   * @param validator Validator used for schema and node registration checks.
   */
  TreeLoader(std::shared_ptr<const BehaviorTreeFactory> factory, TreeParser parser,
             TreeValidator validator);

  /**
   * @brief Loads a behavior tree file from disk.
   *
   * The file type is selected by extension: `.json`, `.yaml`, or `.yml`.
   *
   * @param path Path to the behavior tree document.
   * @param context Runtime-backed context to inject into the tree.
   * @return Tree load result.
   */
  [[nodiscard]] TreeLoadResult LoadFile(const std::filesystem::path& path,
                                        BTContext context = {}) const;

  /**
   * @brief Parses, validates, and constructs a JSON behavior tree.
   *
   * @param document JSON document content.
   * @param context Runtime-backed context to inject into the tree.
   * @return Tree load result.
   */
  [[nodiscard]] TreeLoadResult LoadJson(std::string_view document, BTContext context = {}) const;

  /**
   * @brief Parses, validates, and constructs a YAML behavior tree.
   *
   * @param document YAML document content.
   * @param context Runtime-backed context to inject into the tree.
   * @return Tree load result.
   */
  [[nodiscard]] TreeLoadResult LoadYaml(std::string_view document, BTContext context = {}) const;

private:
  [[nodiscard]] TreeLoadResult Build(TreeParseResult parse_result, BTContext context) const;

  std::shared_ptr<const BehaviorTreeFactory> factory_;
  TreeParser parser_;
  TreeValidator validator_;
};

} // namespace humanoid::bt
