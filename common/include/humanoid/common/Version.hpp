#pragma once

/**
 * @file Version.hpp
 * @brief Defines semantic version information for humanoid-core.
 */

#include <string>

namespace humanoid::common {

/**
 * @brief Represents a semantic version.
 */
class SemanticVersion final {
public:
  /**
   * @brief Constructs a semantic version.
   *
   * @param major_version Major version component.
   * @param minor_version Minor version component.
   * @param patch_version Patch version component.
   */
  constexpr SemanticVersion(int major_version, int minor_version, int patch_version) noexcept
      : major_version_(major_version), minor_version_(minor_version),
        patch_version_(patch_version) {}

  /**
   * @brief Returns the major version component.
   *
   * @return Major version.
   */
  [[nodiscard]] constexpr int major() const noexcept { return major_version_; }

  /**
   * @brief Returns the minor version component.
   *
   * @return Minor version.
   */
  [[nodiscard]] constexpr int minor() const noexcept { return minor_version_; }

  /**
   * @brief Returns the patch version component.
   *
   * @return Patch version.
   */
  [[nodiscard]] constexpr int patch() const noexcept { return patch_version_; }

  /**
   * @brief Reports whether the version is a stable major release.
   *
   * @return True when the major version is at least one.
   */
  [[nodiscard]] constexpr bool stable() const noexcept { return major_version_ > 0; }

  /**
   * @brief Formats the version as major.minor.patch.
   *
   * @return Semantic version string.
   */
  [[nodiscard]] std::string toString() const;

private:
  int major_version_;
  int minor_version_;
  int patch_version_;
};

/**
 * @brief Returns the public API version for this framework package.
 *
 * @return Semantic version for the installed public API.
 */
[[nodiscard]] constexpr SemanticVersion apiVersion() noexcept { return SemanticVersion{0, 1, 0}; }

} // namespace humanoid::common
