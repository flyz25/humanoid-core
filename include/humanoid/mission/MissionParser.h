#pragma once

/**
 * @file MissionParser.h
 * @brief Defines JSON and YAML parsing for mission documents.
 */

#include <string>
#include <string_view>

#include <humanoid/mission/Mission.h>

namespace humanoid::mission {

/**
 * @brief Result returned by mission document parsing.
 */
struct MissionParseResult final {
  /**
   * @brief Parsed mission. Only meaningful when `success` is true.
   */
  Mission mission;

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
   * @return True when the mission was parsed successfully.
   */
  [[nodiscard]] bool Succeeded() const noexcept { return success; }
};

/**
 * @brief Parses mission documents into the vendor-independent mission model.
 *
 * `MissionParser` supports the strict humanoid-core mission JSON schema and a
 * deterministic YAML subset for the same schema. It performs no file I/O and
 * has no robot, SDK, or execution dependency.
 */
class MissionParser final {
public:
  /**
   * @brief Constructs a mission parser.
   */
  MissionParser() = default;

  /**
   * @brief Parses a JSON mission document.
   *
   * @param document JSON document content.
   * @return Parsed mission result.
   */
  [[nodiscard]] MissionParseResult ParseJson(std::string_view document) const;

  /**
   * @brief Parses a YAML mission document.
   *
   * @param document YAML document content.
   * @return Parsed mission result.
   */
  [[nodiscard]] MissionParseResult ParseYaml(std::string_view document) const;
};

} // namespace humanoid::mission
