#include <humanoid/mission/MissionLoader.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

namespace humanoid::mission {
namespace {

[[nodiscard]] MissionLoadResult Result(Mission mission, bool success, std::string message) {
  MissionLoadResult result;
  result.mission = std::move(mission);
  result.success = success;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] MissionLoadResult Failure(std::string message) {
  return Result(Mission{}, false, std::move(message));
}

[[nodiscard]] std::string LowercaseExtension(const std::filesystem::path& path) {
  std::string extension = path.extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
  return extension;
}

[[nodiscard]] MissionLoadResult ValidateParsedMission(MissionParseResult parse_result,
                                                      const MissionValidator& validator) {
  if (!parse_result.Succeeded()) {
    return Failure(parse_result.message);
  }

  const MissionValidationResult validation = validator.Validate(parse_result.mission);
  if (!validation.Succeeded()) {
    return Result(std::move(parse_result.mission), false, validation.message);
  }

  return Result(std::move(parse_result.mission), true, "Mission loaded successfully");
}

} // namespace

MissionLoadResult MissionLoader::LoadFile(const std::filesystem::path& path) const {
  try {
    std::ifstream file{path, std::ios::in | std::ios::binary};
    if (!file.is_open()) {
      return Failure("Unable to open mission file: " + path.string());
    }

    const std::string content{std::istreambuf_iterator<char>{file},
                              std::istreambuf_iterator<char>{}};
    if (file.bad()) {
      return Failure("Unable to read mission file: " + path.string());
    }

    const std::string extension = LowercaseExtension(path);
    if (extension == ".json") {
      return LoadJson(content);
    }
    if (extension == ".yaml" || extension == ".yml") {
      return LoadYaml(content);
    }

    return Failure("Unsupported mission file extension: " + extension);
  } catch (const std::exception& exception) {
    return Failure(std::string{"Mission file load failed: "} + exception.what());
  } catch (...) {
    return Failure("Mission file load failed with an unknown error");
  }
}

MissionLoadResult MissionLoader::LoadJson(std::string_view document) const {
  try {
    return ValidateParsedMission(parser_.ParseJson(document), validator_);
  } catch (const std::exception& exception) {
    return Failure(std::string{"JSON mission load failed: "} + exception.what());
  } catch (...) {
    return Failure("JSON mission load failed with an unknown error");
  }
}

MissionLoadResult MissionLoader::LoadYaml(std::string_view document) const {
  try {
    return ValidateParsedMission(parser_.ParseYaml(document), validator_);
  } catch (const std::exception& exception) {
    return Failure(std::string{"YAML mission load failed: "} + exception.what());
  } catch (...) {
    return Failure("YAML mission load failed with an unknown error");
  }
}

} // namespace humanoid::mission
