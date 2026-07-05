#pragma once

/**
 * @file ExecutionMetadata.h
 * @brief Defines metadata carried by execution runtime contexts.
 */

#include <functional>
#include <map>
#include <string>

namespace humanoid::runtime {

/**
 * @brief Ordered collection of non-operational runtime annotations.
 *
 * Metadata is suitable for correlation identifiers, trace labels, and
 * integration context. It must not carry credentials, vendor objects, or
 * values that silently change execution behavior.
 */
using ExecutionMetadata = std::map<std::string, std::string, std::less<>>;

} // namespace humanoid::runtime
