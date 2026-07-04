#pragma once

/**
 * @file ConfigValue.hpp
 * @brief Defines the supported scalar configuration value types.
 */

#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>

namespace humanoid::configuration {

/**
 * @brief Scalar value that may be returned by a configuration provider.
 */
using ConfigValue = std::variant<bool, std::int64_t, double, std::string, std::filesystem::path>;

} // namespace humanoid::configuration
