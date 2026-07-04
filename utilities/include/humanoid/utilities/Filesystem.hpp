#pragma once

/**
 * @file Filesystem.hpp
 * @brief Defines filesystem helper functions for framework infrastructure.
 */

#include <filesystem>

#include <humanoid/common/Status.hpp>

namespace humanoid::utilities {

/**
 * @brief Ensures a directory exists.
 *
 * @param path Directory path to verify or create.
 * @return Success when the path exists as a directory or is created.
 */
[[nodiscard]] common::Status ensureDirectoryExists(const std::filesystem::path& path);

/**
 * @brief Verifies that a path exists and is a regular file.
 *
 * @param path File path to validate.
 * @return Success when the path exists as a regular file.
 */
[[nodiscard]] common::Status requireRegularFile(const std::filesystem::path& path);

} // namespace humanoid::utilities
