#pragma once

/**
 * @file MissionLoader.h
 * @brief Defines file loading for JSON and YAML mission documents.
 */

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include <humanoid/mission/Mission.h>
#include <humanoid/mission/MissionParser.h>
#include <humanoid/mission/MissionValidator.h>

namespace humanoid::mission {

/**
 * @brief Result returned by mission file loading.
 */
struct MissionLoadResult final {
  /**
   * @brief Loaded mission. Only meaningful when `success` is true.
   */
  Mission mission;

  /**
   * @brief True when loading, parsing, and validation succeeded.
   */
  bool success{false};

  /**
   * @brief Human-readable loading, parsing, or validation diagnostic.
   */
  std::string message;

  /**
   * @brief Reports whether loading succeeded.
   *
   * @return True when a valid mission was loaded.
   */
  [[nodiscard]] bool Succeeded() const noexcept { return success; }
};

/**
 * @brief Loads mission files and validates them before returning a Mission.
 *
 * `MissionLoader` owns parser and validator values through dependency
 * injection. It converts YAML or JSON into the in-memory mission model and does
 * not expose file formats to `MissionExecutor`.
 */
class MissionLoader final {
public:
  /**
   * @brief Constructs a loader with default parser and validator instances.
   */
  MissionLoader() = default;

  /**
   * @brief Constructs a loader with injected parser and validator values.
   *
   * @param parser Parser used for text-to-mission conversion.
   * @param validator Validator used for schema validation.
   */
  MissionLoader(MissionParser parser, MissionValidator validator)
      : parser_(std::move(parser)), validator_(std::move(validator)) {}

  /**
   * @brief Loads a mission file from disk.
   *
   * The file type is selected by extension: `.json`, `.yaml`, or `.yml`.
   *
   * @param path Path to the mission document.
   * @return Mission load result.
   */
  [[nodiscard]] MissionLoadResult LoadFile(const std::filesystem::path& path) const;

  /**
   * @brief Parses and validates an in-memory JSON mission document.
   *
   * @param document JSON document content.
   * @return Mission load result.
   */
  [[nodiscard]] MissionLoadResult LoadJson(std::string_view document) const;

  /**
   * @brief Parses and validates an in-memory YAML mission document.
   *
   * @param document YAML document content.
   * @return Mission load result.
   */
  [[nodiscard]] MissionLoadResult LoadYaml(std::string_view document) const;

private:
  MissionParser parser_;
  MissionValidator validator_;
};

} // namespace humanoid::mission
