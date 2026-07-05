#pragma once

/**
 * @file TreeParser.h
 * @brief Defines JSON and YAML parsing for behavior tree documents.
 */

#include <string>
#include <string_view>

#include <humanoid/bt/TreeValidator.h>

namespace humanoid::bt {

/**
 * @brief Result returned by behavior tree document parsing.
 */
struct TreeParseResult final {
  /**
   * @brief Parsed behavior tree document. Only meaningful when `success` is true.
   */
  TreeDocument document;

  /**
   * @brief True when parsing and schema conversion succeeded.
   */
  bool success{false};

  /**
   * @brief Human-readable parsing or conversion diagnostic.
   */
  std::string message;

  /**
   * @brief Reports whether parsing succeeded.
   *
   * @return True when parsing succeeded.
   */
  [[nodiscard]] bool Succeeded() const noexcept { return success; }
};

/**
 * @brief Parses behavior tree documents into vendor-independent definitions.
 *
 * `TreeParser` supports strict JSON and a deterministic YAML subset for the
 * same schema. It performs no file I/O, factory lookup, execution, robot
 * adapter calls, plugin loading, or SDK work.
 */
class TreeParser final {
public:
  /**
   * @brief Constructs a behavior tree parser.
   */
  TreeParser() = default;

  /**
   * @brief Parses a JSON behavior tree document.
   *
   * @param document JSON document content.
   * @return Parsed tree document.
   */
  [[nodiscard]] TreeParseResult ParseJson(std::string_view document) const;

  /**
   * @brief Parses a YAML behavior tree document.
   *
   * @param document YAML document content.
   * @return Parsed tree document.
   */
  [[nodiscard]] TreeParseResult ParseYaml(std::string_view document) const;
};

} // namespace humanoid::bt
