/**
 * @file mission_loader_unit_test.cpp
 * @brief Validates mission YAML and JSON loading without robot hardware.
 */

#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <humanoid/mission/MissionLoader.h>

namespace {

constexpr std::string_view kValidJsonMission = R"json(
{
  "id": 101,
  "name": "JSON mission",
  "description": "Loaded from JSON",
  "version": "1.0.0",
  "author": "humanoid-core",
  "metadata": {
    "source": "unit-test"
  },
  "steps": [
    {
      "id": 1,
      "name": "Stop",
      "timeout_ms": 500,
      "retry": 1,
      "enabled": true,
      "metadata": {
        "phase": "safety"
      },
      "command": {
        "id": 1001,
        "type": "Stop",
        "priority": "High",
        "timeout_ms": 250,
        "metadata": {
          "origin": "json"
        }
      }
    }
  ]
}
)json";

constexpr std::string_view kValidYamlMission = R"yaml(
mission:
  id: 102
  name: YAML mission
  description: Loaded from YAML
  version: 1.0.0
  author: humanoid-core
  metadata:
    source: unit-test
  steps:
    - id: 1
      name: Stop
      timeout_ms: 500
      retry: 0
      enabled: true
      metadata:
        phase: safety
      command:
        id: 2001
        type: Stop
        priority: Critical
        timeout_ms: 250
        metadata:
          origin: yaml
)yaml";

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

[[nodiscard]] std::filesystem::path TestFilePath(std::string_view filename) {
  return std::filesystem::temp_directory_path() / std::string{filename};
}

void WriteFile(const std::filesystem::path& path, std::string_view content) {
  std::ofstream file{path, std::ios::out | std::ios::binary | std::ios::trunc};
  if (!file.is_open()) {
    throw std::runtime_error{"Unable to create test mission file"};
  }
  file << content;
}

void TestLoadJson() {
  const humanoid::mission::MissionLoader loader;
  const humanoid::mission::MissionLoadResult result = loader.LoadJson(kValidJsonMission);

  Check(result.Succeeded(), "Valid JSON mission did not load");
  Check(result.mission.id == 101U, "JSON mission id mismatch");
  Check(result.mission.name == "JSON mission", "JSON mission name mismatch");
  Check(result.mission.metadata.at("source") == "unit-test", "JSON metadata mismatch");
  Check(result.mission.steps.size() == 1U, "JSON step count mismatch");
  Check(result.mission.steps.front().command.type == humanoid::core::CommandType::Stop,
        "JSON command type mismatch");
  Check(result.mission.steps.front().command.priority == humanoid::core::CommandPriority::High,
        "JSON command priority mismatch");
}

void TestLoadYaml() {
  const humanoid::mission::MissionLoader loader;
  const humanoid::mission::MissionLoadResult result = loader.LoadYaml(kValidYamlMission);

  Check(result.Succeeded(), "Valid YAML mission did not load");
  Check(result.mission.id == 102U, "YAML mission id mismatch");
  Check(result.mission.name == "YAML mission", "YAML mission name mismatch");
  Check(result.mission.steps.size() == 1U, "YAML step count mismatch");
  Check(result.mission.steps.front().metadata.at("phase") == "safety", "YAML metadata mismatch");
  Check(result.mission.steps.front().command.priority == humanoid::core::CommandPriority::Critical,
        "YAML command priority mismatch");
}

void TestInvalidYaml() {
  const humanoid::mission::MissionLoader loader;
  const humanoid::mission::MissionLoadResult result = loader.LoadYaml("id: 1\n\tname: invalid\n");

  Check(!result.Succeeded(), "Invalid YAML was accepted");
}

void TestMissingRequiredField() {
  constexpr std::string_view missing_name = R"json(
{
  "id": 103,
  "description": "Missing name",
  "version": "1.0.0",
  "author": "humanoid-core",
  "steps": [
    {
      "id": 1,
      "name": "Stop",
      "command": {
        "id": 3001,
        "type": "Stop"
      }
    }
  ]
}
)json";

  const humanoid::mission::MissionLoader loader;
  const humanoid::mission::MissionLoadResult result = loader.LoadJson(missing_name);

  Check(!result.Succeeded(), "Mission with missing required field was accepted");
}

void TestUnknownCommand() {
  constexpr std::string_view unknown_command = R"yaml(
id: 104
name: Unknown command
description: Invalid command type
version: 1.0.0
author: humanoid-core
steps:
  - id: 1
    name: Unknown
    command:
      id: 4001
      type: Fly
)yaml";

  const humanoid::mission::MissionLoader loader;
  const humanoid::mission::MissionLoadResult result = loader.LoadYaml(unknown_command);

  Check(!result.Succeeded(), "Mission with unknown command was accepted");
}

void TestLoadFile() {
  const std::filesystem::path path = TestFilePath("humanoid_core_mission_loader_test.json");
  WriteFile(path, kValidJsonMission);

  const humanoid::mission::MissionLoader loader;
  const humanoid::mission::MissionLoadResult result = loader.LoadFile(path);
  std::filesystem::remove(path);

  Check(result.Succeeded(), "Mission file did not load");
  Check(result.mission.id == 101U, "Loaded mission file id mismatch");
}

} // namespace

int main() {
  try {
    TestLoadJson();
    TestLoadYaml();
    TestInvalidYaml();
    TestMissingRequiredField();
    TestUnknownCommand();
    TestLoadFile();
  } catch (...) {
    return 1;
  }

  return 0;
}
