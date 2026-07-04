#include <humanoid/adapters/IRobotAdapter.h>
#include <humanoid/logging/LoggerManager.hpp>

#include <factory/RobotFactoryRegistry.h>

#if HUMANOID_CORE_HAS_UNITREE
#include <adapters/unitree/UnitreeRobotFactory.h>
#endif

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

/**
 * @brief Trims leading and trailing ASCII whitespace.
 *
 * @param value Input text.
 * @return Trimmed text.
 */
std::string Trim(const std::string& value) {
  const std::string whitespace = " \t\r\n";
  const std::size_t first = value.find_first_not_of(whitespace);
  if (first == std::string::npos) {
    return {};
  }

  const std::size_t last = value.find_last_not_of(whitespace);
  return value.substr(first, last - first + 1);
}

/**
 * @brief Removes an inline comment from a simple scalar line.
 *
 * @param value Input text.
 * @return Text before a comment marker.
 */
std::string StripComment(const std::string& value) {
  const std::size_t comment = value.find('#');
  return comment == std::string::npos ? value : value.substr(0, comment);
}

/**
 * @brief Parses a non-negative integer.
 *
 * @param value Scalar text.
 * @param key Key name used for diagnostics.
 * @return Parsed integer.
 */
std::uint32_t ParseUnsigned(const std::string& value, const std::string& key) {
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
 * @return Key-value pair when present.
 */
std::optional<std::pair<std::string, std::string>> ParseKeyValue(const std::string& line) {
  const std::size_t separator = line.find(':');
  if (separator == std::string::npos) {
    return std::nullopt;
  }

  const std::string key = Trim(line.substr(0, separator));
  const std::string value = Trim(line.substr(separator + 1));
  if (key.empty()) {
    return std::nullopt;
  }

  return std::make_pair(key, value);
}

/**
 * @brief Loads the narrow robot.yaml schema used by this example.
 *
 * This is intentionally not a general YAML parser and is scoped only to the
 * documented robot connection keys.
 *
 * @param path Configuration file path.
 * @return Robot configuration.
 */
humanoid::adapters::RobotConfig LoadRobotConfig(const std::filesystem::path& path) {
  std::ifstream input{path};
  if (!input) {
    throw std::runtime_error("failed to open config file: " + path.string());
  }

  humanoid::adapters::RobotConfig config;
  bool in_robot_section = false;

  std::string line;
  while (std::getline(input, line)) {
    const std::string trimmed = Trim(StripComment(line));
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

    const std::optional<std::pair<std::string, std::string>> key_value = ParseKeyValue(trimmed);
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
      config.timeout = std::chrono::milliseconds{ParseUnsigned(value, key)};
    } else if (key == "domain_id") {
      config.domain_id = ParseUnsigned(value, key);
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
 * @brief Throws if a command failed.
 *
 * @param result Command result.
 * @param command Command name.
 */
void RequireSuccess(const humanoid::adapters::Result& result, const std::string& command) {
  if (!result.Succeeded()) {
    throw std::runtime_error(command + " failed: " + result.message);
  }
}

/**
 * @brief Reports whether physical robot commands should be executed.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return True when --execute is present.
 */
bool ShouldExecuteRobotCommands(int argc, char* argv[]) {
  for (int index = 1; index < argc; ++index) {
    if (std::string{argv[index]} == "--execute") {
      return true;
    }
  }

  return false;
}

/**
 * @brief Returns the config path from arguments.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Config path.
 */
std::filesystem::path ConfigPath(int argc, char* argv[]) {
  for (int index = 1; index < argc; ++index) {
    const std::string argument{argv[index]};
    if (argument != "--execute") {
      return std::filesystem::path{argument};
    }
  }

  return std::filesystem::path{"config/robot.yaml"};
}

} // namespace

int main(int argc, char* argv[]) {
  try {
    const humanoid::adapters::RobotConfig config = LoadRobotConfig(ConfigPath(argc, argv));
    auto logger = std::make_shared<humanoid::logging::LoggerManager>();

    humanoid::factory::RobotFactoryRegistry registry;

#if HUMANOID_CORE_HAS_UNITREE
    RequireSuccess(registry.RegisterFactory(
                       std::make_shared<humanoid::adapters::unitree::UnitreeRobotFactory>()),
                   "RegisterFactory");
#endif

    std::unique_ptr<humanoid::adapters::IRobotAdapter> adapter =
        registry.CreateAdapter(config, logger);
    if (!adapter) {
      std::cout << "Robot adapter unavailable for " << config.vendor << ' ' << config.model
                << "; exiting gracefully" << '\n';
      return EXIT_SUCCESS;
    }

    if (!ShouldExecuteRobotCommands(argc, argv)) {
      std::cout << "Robot adapter created for " << config.vendor << ' ' << config.model
                << ". Pass --execute to command physical hardware." << '\n';
      return EXIT_SUCCESS;
    }

    RequireSuccess(adapter->Initialize(), "Initialize");
    RequireSuccess(adapter->Connect(), "Connect");
    RequireSuccess(adapter->StandUp(), "StandUp");
    RequireSuccess(adapter->BalanceStand(), "BalanceStand");
    RequireSuccess(adapter->Disconnect(), "Disconnect");
    RequireSuccess(adapter->Shutdown(), "Shutdown");

    std::cout << "Robot connection workflow completed" << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
