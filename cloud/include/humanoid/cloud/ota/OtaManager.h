#pragma once

/**
 * @file OtaManager.h
 * @brief Defines optional OTA package and update planning primitives.
 */

#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <humanoid/cloud/CloudTypes.h>

namespace humanoid::cloud::ota {

/**
 * @brief OTA package category.
 */
enum class PackageType : unsigned char {
  Software,     ///< Framework or application software.
  Plugin,       ///< Robot or platform plugin.
  Mission,      ///< Mission package.
  Configuration ///< Configuration package.
};

/**
 * @brief OTA update state.
 */
enum class OtaStatus : unsigned char {
  Planned,    ///< Update is planned.
  Verified,   ///< Package verification passed.
  Applied,    ///< Update was applied.
  RolledBack, ///< Update was rolled back.
  Rejected    ///< Update was rejected.
};

/**
 * @brief OTA package descriptor.
 */
struct PackageDescriptor final {
  /** @brief Stable package identifier. */
  std::string packageId;

  /** @brief Semantic version string. */
  std::string version;

  /** @brief Package type. */
  PackageType type{PackageType::Software};

  /** @brief Expected package checksum or signature. */
  std::string verificationDigest;

  /** @brief True when rollback metadata is available. */
  bool rollbackSupported{false};
};

/**
 * @brief OTA update plan.
 */
struct UpdatePlan final {
  /** @brief Target robot identifiers. */
  std::vector<std::string> robotIds;

  /** @brief Package descriptor. */
  PackageDescriptor package;

  /** @brief Current update state. */
  OtaStatus status{OtaStatus::Planned};

  /** @brief Plan timestamp. */
  humanoid::cloud::CloudTimestamp timestamp{};
};

/**
 * @brief Thread-safe OTA registry and plan manager.
 */
class OtaManager final {
public:
  /** @brief Registers a package descriptor. */
  [[nodiscard]] humanoid::cloud::CloudResult RegisterPackage(PackageDescriptor package);

  /** @brief Creates an update plan for a registered package. */
  [[nodiscard]] humanoid::cloud::CloudResult PlanUpdate(const std::string& package_id,
                                                        std::vector<std::string> robot_ids);

  /** @brief Marks an update package as verified for all matching plans. */
  [[nodiscard]] humanoid::cloud::CloudResult VerifyPackage(const std::string& package_id,
                                                           const std::string& digest);

  /** @brief Rolls back all matching update plans when supported. */
  [[nodiscard]] humanoid::cloud::CloudResult Rollback(const std::string& package_id);

  /** @brief Returns registered packages. */
  [[nodiscard]] std::vector<PackageDescriptor> Packages() const;

  /** @brief Returns update plans. */
  [[nodiscard]] std::vector<UpdatePlan> Plans() const;

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, PackageDescriptor> packages_;
  std::vector<UpdatePlan> plans_;
};

} // namespace humanoid::cloud::ota
