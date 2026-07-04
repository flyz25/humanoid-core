#pragma once

/**
 * @file RuntimeMetadata.hpp
 * @brief Defines runtime metadata for applications using humanoid-core.
 */

#include <filesystem>
#include <string>
#include <string_view>

#include <humanoid/common/Version.hpp>

namespace humanoid::core {

/**
 * @brief Describes an application instance built on humanoid-core.
 */
class RuntimeMetadata final {
public:
  /**
   * @brief Constructs runtime metadata.
   *
   * @param application_name Application name.
   * @param root_path Application root path.
   */
  RuntimeMetadata(std::string application_name, std::filesystem::path root_path);

  /**
   * @brief Returns the framework name.
   *
   * @return Stable framework name.
   */
  [[nodiscard]] static constexpr std::string_view frameworkName() noexcept {
    return "humanoid-core";
  }

  /**
   * @brief Returns the application name.
   *
   * @return Application name.
   */
  [[nodiscard]] const std::string& applicationName() const noexcept;

  /**
   * @brief Returns the application root path.
   *
   * @return Application root path.
   */
  [[nodiscard]] const std::filesystem::path& rootPath() const noexcept;

  /**
   * @brief Returns the linked humanoid-core API version.
   *
   * @return Semantic API version.
   */
  [[nodiscard]] common::SemanticVersion apiVersion() const noexcept;

private:
  std::string application_name_;
  std::filesystem::path root_path_;
};

} // namespace humanoid::core
