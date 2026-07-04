#pragma once

/**
 * @file PluginVersionCompatibility.hpp
 * @brief Defines framework API compatibility rules for plugins.
 */

#include <humanoid/common/Version.hpp>

namespace humanoid::plugins {

/**
 * @brief Minimum framework API version supported by the plugin infrastructure.
 */
inline constexpr humanoid::common::SemanticVersion kMinimumPluginFrameworkVersion{
    humanoid::common::apiVersion()};

/**
 * @brief Maximum framework API version supported by the plugin infrastructure.
 */
inline constexpr humanoid::common::SemanticVersion kMaximumPluginFrameworkVersion{
    humanoid::common::apiVersion()};

/**
 * @brief Describes the framework API version range accepted by a plugin.
 */
class PluginVersionCompatibility final {
public:
  /**
   * @brief Constructs compatibility for the current humanoid-core API version.
   */
  constexpr PluginVersionCompatibility() noexcept = default;

  /**
   * @brief Constructs compatibility for an inclusive framework API range.
   *
   * @param minimum_framework_version Minimum accepted framework API version.
   * @param maximum_framework_version Maximum accepted framework API version.
   */
  constexpr PluginVersionCompatibility(
      humanoid::common::SemanticVersion minimum_framework_version,
      humanoid::common::SemanticVersion maximum_framework_version) noexcept
      : minimum_framework_version_(minimum_framework_version),
        maximum_framework_version_(maximum_framework_version) {}

  /**
   * @brief Returns the minimum accepted framework API version.
   *
   * @return Minimum accepted framework API version.
   */
  [[nodiscard]] constexpr humanoid::common::SemanticVersion
  minimumFrameworkVersion() const noexcept {
    return minimum_framework_version_;
  }

  /**
   * @brief Returns the maximum accepted framework API version.
   *
   * @return Maximum accepted framework API version.
   */
  [[nodiscard]] constexpr humanoid::common::SemanticVersion
  maximumFrameworkVersion() const noexcept {
    return maximum_framework_version_;
  }

  /**
   * @brief Reports whether a framework API version is within the accepted range.
   *
   * @param framework_version Framework API version to check.
   * @return True when the framework API version is compatible.
   */
  [[nodiscard]] constexpr bool
  IsCompatibleWith(humanoid::common::SemanticVersion framework_version) const noexcept {
    return Compare(minimum_framework_version_, framework_version) <= 0 &&
           Compare(framework_version, maximum_framework_version_) <= 0;
  }

  /**
   * @brief Reports whether the configured version range is internally valid.
   *
   * @return True when minimum version is not greater than maximum version.
   */
  [[nodiscard]] constexpr bool IsValid() const noexcept {
    return Compare(minimum_framework_version_, maximum_framework_version_) <= 0;
  }

private:
  /**
   * @brief Compares two semantic versions.
   *
   * @param lhs Left-hand semantic version.
   * @param rhs Right-hand semantic version.
   * @return Negative if lhs is older, zero if equal, positive if lhs is newer.
   */
  [[nodiscard]] static constexpr int
  Compare(humanoid::common::SemanticVersion lhs,
          humanoid::common::SemanticVersion rhs) noexcept {
    if (lhs.major() != rhs.major()) {
      return lhs.major() < rhs.major() ? -1 : 1;
    }
    if (lhs.minor() != rhs.minor()) {
      return lhs.minor() < rhs.minor() ? -1 : 1;
    }
    if (lhs.patch() != rhs.patch()) {
      return lhs.patch() < rhs.patch() ? -1 : 1;
    }
    return 0;
  }

  humanoid::common::SemanticVersion minimum_framework_version_{
      kMinimumPluginFrameworkVersion};
  humanoid::common::SemanticVersion maximum_framework_version_{
      kMaximumPluginFrameworkVersion};
};

} // namespace humanoid::plugins
