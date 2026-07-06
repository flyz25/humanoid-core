#pragma once

/**
 * @file CloudPlatform.h
 * @brief Aggregates optional cloud platform services through dependency injection.
 */

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>

#include <humanoid/cloud/api/RestApiCatalog.h>
#include <humanoid/cloud/auth/AuthManager.h>
#include <humanoid/cloud/fleet/FleetManager.h>
#include <humanoid/cloud/monitoring/ObservabilityRegistry.h>
#include <humanoid/cloud/ota/OtaManager.h>

namespace humanoid::cloud {

/**
 * @brief Cloud platform dependencies.
 */
struct CloudPlatformDependencies final {
  /** @brief REST API endpoint catalog. */
  std::shared_ptr<api::RestApiCatalog> restApiCatalog;

  /** @brief Fleet registry and status manager. */
  std::shared_ptr<fleet::FleetManager> fleetManager;

  /** @brief Optional authentication manager. */
  std::shared_ptr<auth::AuthManager> authManager;

  /** @brief Optional OTA manager. */
  std::shared_ptr<ota::OtaManager> otaManager;

  /** @brief Optional observability registry. */
  std::shared_ptr<monitoring::ObservabilityRegistry> observability;
};

/**
 * @brief Lifecycle status of the optional cloud platform facade.
 */
struct CloudPlatformStatus final {
  /** @brief True when the platform is running. */
  bool running{false};

  /** @brief API version exposed by the platform. */
  std::string apiVersion{"v1"};

  /** @brief Number of registered REST endpoints. */
  std::size_t restEndpointCount{0U};

  /** @brief Number of registered robots. */
  std::size_t robotCount{0U};
};

/**
 * @brief Thread-safe optional cloud platform facade.
 *
 * This class owns no HTTP server, gRPC server, database, cloud SDK, or
 * authentication SDK. Production deployments may wrap it with concrete
 * adapters that translate external requests into framework-owned contracts.
 */
class CloudPlatform final {
public:
  /**
   * @brief Constructs the platform facade with injected dependencies.
   *
   * Missing dependencies are replaced with default in-memory services.
   *
   * @param dependencies Optional dependency set.
   */
  explicit CloudPlatform(CloudPlatformDependencies dependencies = {});

  /** @brief Stops the platform facade. */
  ~CloudPlatform() noexcept;

  CloudPlatform(const CloudPlatform&) = delete;
  CloudPlatform& operator=(const CloudPlatform&) = delete;
  CloudPlatform(CloudPlatform&&) = delete;
  CloudPlatform& operator=(CloudPlatform&&) = delete;

  /** @brief Starts the platform lifecycle. */
  [[nodiscard]] bool Start();

  /** @brief Stops the platform lifecycle. */
  void Stop() noexcept;

  /** @brief Returns current lifecycle and registry status. */
  [[nodiscard]] CloudPlatformStatus Status() const;

  /** @brief Returns the REST API catalog. */
  [[nodiscard]] std::shared_ptr<api::RestApiCatalog> RestApi() const noexcept;

  /** @brief Returns the fleet manager. */
  [[nodiscard]] std::shared_ptr<fleet::FleetManager> Fleet() const noexcept;

  /** @brief Returns the auth manager. */
  [[nodiscard]] std::shared_ptr<auth::AuthManager> Auth() const noexcept;

  /** @brief Returns the OTA manager. */
  [[nodiscard]] std::shared_ptr<ota::OtaManager> Ota() const noexcept;

  /** @brief Returns the observability registry. */
  [[nodiscard]] std::shared_ptr<monitoring::ObservabilityRegistry> Observability() const noexcept;

private:
  CloudPlatformDependencies dependencies_;
  mutable std::mutex mutex_;
  bool running_{false};
};

} // namespace humanoid::cloud
