#pragma once

/**
 * @file AuthManager.h
 * @brief Defines optional authentication, RBAC, and audit logging primitives.
 */

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <humanoid/cloud/CloudTypes.h>

namespace humanoid::cloud::auth {

/**
 * @brief Supported authentication schemes.
 */
enum class AuthScheme : unsigned char {
  ApiKey, ///< API key authentication.
  Jwt,    ///< JSON Web Token authentication.
  OAuth2  ///< OAuth2 bearer authentication.
};

/**
 * @brief Authenticated principal.
 */
struct Principal final {
  /** @brief Principal identifier. */
  std::string principalId;

  /** @brief Assigned role names. */
  std::vector<std::string> roles;

  /** @brief Authentication scheme used for this principal. */
  AuthScheme scheme{AuthScheme::ApiKey};
};

/**
 * @brief Audit log entry.
 */
struct AuditRecord final {
  /** @brief Principal identifier. */
  std::string principalId;

  /** @brief Action attempted by the principal. */
  std::string action;

  /** @brief True when the action was authorized. */
  bool authorized{false};

  /** @brief Audit timestamp. */
  humanoid::cloud::CloudTimestamp timestamp{};
};

/**
 * @brief Thread-safe optional auth manager.
 */
class AuthManager final {
public:
  /** @brief Registers an API key principal. */
  [[nodiscard]] humanoid::cloud::CloudResult RegisterApiKey(std::string api_key,
                                                            Principal principal);

  /** @brief Grants an action permission to a role. */
  [[nodiscard]] humanoid::cloud::CloudResult GrantPermission(std::string role, std::string action);

  /** @brief Authorizes an API key for an action and records audit state. */
  [[nodiscard]] bool AuthorizeApiKey(const std::string& api_key, const std::string& action);

  /** @brief Returns audit records. */
  [[nodiscard]] std::vector<AuditRecord> AuditLog() const;

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, Principal> api_keys_;
  std::unordered_map<std::string, std::unordered_set<std::string>> role_permissions_;
  std::vector<AuditRecord> audit_log_;
};

} // namespace humanoid::cloud::auth
