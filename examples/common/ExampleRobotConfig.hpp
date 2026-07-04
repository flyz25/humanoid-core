#pragma once

/**
 * @file ExampleRobotConfig.hpp
 * @brief Provides narrow robot.yaml loading helpers for examples.
 */

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <humanoid/adapters/IRobotAdapter.h>

namespace humanoid::examples {

/**
 * @brief Trims leading and trailing ASCII whitespace.
 *
 * @param value Input text.
 * @return Trimmed text.
 */
[[nodiscard]] inline std::string TrimAsciiWhitespace(const std::string& value) {
  const std::string whitespace = " \t\r\n";
  const std::size_t first = value.find_first_not_of(whitespace);
  if (first == std::string::npos) {
    return {};
  }

  const std::size_t last = value.find_last_not_of(whitespace);
  return value.substr(first, last - first + 1U);
}

/**
 * @brief Removes an inline comment from a simple scalar line.
 *
 * @param value Input text.
 * @return Text before the comment marker.
 */
[[nodiscard]] inline std::string StripYamlComment(const std::string& value) {
  const std::size_t comment = value.find('#');
  return comment == std::string::npos ? value : value.substr(0, comment);
}

/**
 * @brief Parses a non-negative integer scalar.
 *
 * @param value Scalar text.
 * @param key Key name used for diagnostics.
 * @return Parsed integer.
 */
[[nodiscard]] inline std::uint32_t ParseUnsignedScalar(const std::string& value,
                                                       const std::string& key) {
  std::size_t consumed = 0;
  const unsigned long parsed = std::stoul(value, &consumed, 10);
  if (consumed != value.size()) {
    throw std::runtime_error("invalid integer value for " + key);
  }

  if (parsed > static_cast<unsigned long>(std::numeric_limits<std::uint32_t>::max())) {
    throw std::runtime_error("integer value out of range for " + key);
  }

  return static_cast<std::uint32_t>(parsed);
}

/**
 * @brief Parses a scalar key-value line of the form key: value.
 *
 * @param line Input line.
 * @return Parsed key-value pair when present.
 */
[[nodiscard]] inline std::optional<std::pair<std::string, std::string>>
ParseScalarKeyValue(const std::string& line) {
  const std::size_t separator = line.find(':');
  if (separator == std::string::npos) {
    return std::nullopt;
  }

  const std::string key = TrimAsciiWhitespace(line.substr(0, separator));
  const std::string value = TrimAsciiWhitespace(line.substr(separator + 1U));
  if (key.empty()) {
    return std::nullopt;
  }

  return std::make_pair(key, value);
}

/**
 * @brief Loads the narrow robot.yaml schema used by examples.
 *
 * This function is intentionally not a general YAML parser. It supports only
 * scalar fields from the repository's documented `config/robot.yaml` shape.
 *
 * @param path Configuration file path.
 * @return Robot configuration.
 */
[[nodiscard]] inline humanoid::adapters::RobotConfig
LoadRobotConfig(const std::filesystem::path& path) {
  std::ifstream input{path};
  if (!input) {
    throw std::runtime_error("failed to open config file: " + path.string());
  }

  humanoid::adapters::RobotConfig config;
  bool in_robot_section = false;

  std::string line;
  while (std::getline(input, line)) {
    const std::string trimmed = TrimAsciiWhitespace(StripYamlComment(line));
    if (trimmed.empty()) {
      continue;
    }

    if (trimmed == "robot:") {
      in_robot_section = true;
      continue;
    }

    if (!in_robot_section) {
      continue;
    }

    const std::optional<std::pair<std::string, std::string>> key_value =
        ParseScalarKeyValue(trimmed);
    if (!key_value.has_value()) {
      throw std::runtime_error("invalid robot config line: " + trimmed);
    }

    const std::string& key = key_value->first;
    const std::string& value = key_value->second;
    if (key == "vendor") {
      config.vendor = value;
    } else if (key == "model") {
      config.model = value;
    } else if (key == "ip") {
      config.ip = value;
    } else if (key == "network_interface") {
      config.network_interface = value;
    } else if (key == "timeout_ms") {
      config.timeout = std::chrono::milliseconds{ParseUnsignedScalar(value, key)};
    } else if (key == "domain_id") {
      config.domain_id = ParseUnsignedScalar(value, key);
    } else if (key == "serial_number") {
      config.serial_number = value;
    } else if (key == "firmware") {
      config.firmware = value;
    } else {
      throw std::runtime_error("unsupported robot config key: " + key);
    }
  }

  if (config.vendor.empty()) {
    throw std::runtime_error("robot.vendor is required");
  }

  if (config.model.empty()) {
    throw std::runtime_error("robot.model is required");
  }

  return config;
}

/**
 * @brief Reports whether an argument vector contains a flag.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param flag Flag to search for.
 * @return True when the flag is present.
 */
[[nodiscard]] inline bool HasArgumentFlag(int argc, char* argv[], std::string_view flag) {
  for (int index = 1; index < argc; ++index) {
    if (std::string_view{argv[index]} == flag) {
      return true;
    }
  }

  return false;
}

/**
 * @brief Returns the first non-flag argument as a config path.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param default_path Default path used when no positional path is present.
 * @return Configuration path.
 */
[[nodiscard]] inline std::filesystem::path
ConfigPathFromArguments(int argc, char* argv[], std::filesystem::path default_path) {
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (!argument.empty() && argument.front() != '-') {
      return std::filesystem::path{argument};
    }
  }

  return default_path;
}

} // namespace humanoid::examples
