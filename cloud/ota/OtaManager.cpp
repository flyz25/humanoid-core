#include <humanoid/cloud/ota/OtaManager.h>

#include <chrono>
#include <mutex>
#include <utility>

namespace humanoid::cloud::ota {
namespace {

[[nodiscard]] humanoid::cloud::CloudTimestamp now() noexcept {
  return humanoid::cloud::CloudTimestamp{std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch())};
}

} // namespace

humanoid::cloud::CloudResult OtaManager::RegisterPackage(PackageDescriptor package) {
  if (package.packageId.empty() || package.version.empty()) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::InvalidInput,
                                    "package id and version are required");
  }

  std::unique_lock lock{mutex_};
  packages_[package.packageId] = std::move(package);
  return humanoid::cloud::Success("package registered");
}

humanoid::cloud::CloudResult OtaManager::PlanUpdate(const std::string& package_id,
                                                    std::vector<std::string> robot_ids) {
  if (robot_ids.empty()) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::InvalidInput,
                                    "target robots are required");
  }

  std::unique_lock lock{mutex_};
  const auto package = packages_.find(package_id);
  if (package == packages_.end()) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::NotFound,
                                    "package is not registered");
  }

  plans_.push_back(UpdatePlan{std::move(robot_ids), package->second, OtaStatus::Planned, now()});
  return humanoid::cloud::Success("update planned");
}

humanoid::cloud::CloudResult OtaManager::VerifyPackage(const std::string& package_id,
                                                       const std::string& digest) {
  std::unique_lock lock{mutex_};
  const auto package = packages_.find(package_id);
  if (package == packages_.end()) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::NotFound,
                                    "package is not registered");
  }
  if (package->second.verificationDigest != digest) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::InvalidInput,
                                    "package verification digest does not match");
  }

  for (auto& plan : plans_) {
    if (plan.package.packageId == package_id) {
      plan.status = OtaStatus::Verified;
    }
  }
  return humanoid::cloud::Success("package verified");
}

humanoid::cloud::CloudResult OtaManager::Rollback(const std::string& package_id) {
  std::unique_lock lock{mutex_};
  bool found{false};
  for (auto& plan : plans_) {
    if (plan.package.packageId == package_id) {
      found = true;
      if (!plan.package.rollbackSupported) {
        return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::InvalidInput,
                                        "rollback is not supported by package");
      }
      plan.status = OtaStatus::RolledBack;
    }
  }

  if (!found) {
    return humanoid::cloud::Failure(humanoid::cloud::CloudErrorCode::NotFound,
                                    "update plan is not registered");
  }
  return humanoid::cloud::Success("rollback planned");
}

std::vector<PackageDescriptor> OtaManager::Packages() const {
  std::shared_lock lock{mutex_};
  std::vector<PackageDescriptor> packages;
  packages.reserve(packages_.size());
  for (const auto& package : packages_) {
    packages.push_back(package.second);
  }
  return packages;
}

std::vector<UpdatePlan> OtaManager::Plans() const {
  std::shared_lock lock{mutex_};
  return plans_;
}

} // namespace humanoid::cloud::ota
