#pragma once

/**
 * @file MissionMetadata.h
 * @brief Defines metadata types used by vendor-independent missions.
 */

#include <functional>
#include <map>
#include <string>

namespace humanoid::mission {

/**
 * @brief Ordered collection of non-operational mission annotations.
 *
 * Metadata may carry labels, correlation identifiers, or authoring context. It
 * must not contain vendor SDK objects, robot handles, credentials, or values
 * that silently alter command execution behavior.
 */
using MissionMetadata = std::map<std::string, std::string, std::less<>>;

} // namespace humanoid::mission
