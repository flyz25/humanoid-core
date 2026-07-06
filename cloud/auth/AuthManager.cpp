#include <humanoid/cloud/auth/AuthManager.h>

#include <algorithm>
#include <chrono>
#include <mutex>
#include <utility>

namespace humanoid::cloud::auth {
namespace {

[[nodiscard]] humanoid::cloud::CloudTimestamp now() noexcept {
  return humanoid::cloud::CloudTimestamp{std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch())};
}

} // namespace

humanoid::cloud::CloudResult AuthManager::RegisterApiKey(std::string api_key, Principal principal) {
  if (api_key.empty() || principal.principalId.empty()) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::InvalidInput,
                                    "api key and principal id are required");
  }

  std::unique_lock lock{mutex_};
  api_keys_[std::move(api_key)] = std::move(principal);
  return humanoid::cloud::Success("api key registered");
}

humanoid::cloud::CloudResult AuthManager::GrantPermission(std::string role, std::string action) {
  if (role.empty() || action.empty()) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::InvalidInput,
                                    "role and action are required");
  }

  std::unique_lock lock{mutex_};
  role_permissions_[std::move(role)].insert(std::move(action));
  return humanoid::cloud::Success("permission granted");
}

bool AuthManager::AuthorizeApiKey(const std::string& api_key, const std::string& action) {
  std::unique_lock lock{mutex_};
  const auto principal = api_keys_.find(api_key);
  bool authorized{false};
  std::string principal_id{"anonymous"};

  if (principal != api_keys_.end()) {
    principal_id = principal->second.principalId;
    authorized = std::any_of(principal->second.roles.begin(), principal->second.roles.end(),
                             [this, &action](const std::string& role) {
                               const auto permissions = role_permissions_.find(role);
                               return permissions != role_permissions_.end() &&
                                      permissions->second.contains(action);
                             });
  }

  audit_log_.push_back(AuditRecord{std::move(principal_id), action, authorized, now()});
  return authorized;
}

std::vector<AuditRecord> AuthManager::AuditLog() const {
  std::shared_lock lock{mutex_};
  return audit_log_;
}

} // namespace humanoid::cloud::auth
