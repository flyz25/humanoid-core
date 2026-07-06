#include <humanoid/cloud/CloudPlatform.h>

#include <mutex>
#include <utility>

namespace humanoid::cloud {

CloudPlatform::CloudPlatform(CloudPlatformDependencies dependencies)
    : dependencies_(std::move(dependencies)) {
  if (dependencies_.restApiCatalog == nullptr) {
    dependencies_.restApiCatalog = std::make_shared<api::RestApiCatalog>();
  }
  if (dependencies_.fleetManager == nullptr) {
    dependencies_.fleetManager = std::make_shared<fleet::FleetManager>();
  }
  if (dependencies_.authManager == nullptr) {
    dependencies_.authManager = std::make_shared<auth::AuthManager>();
  }
  if (dependencies_.otaManager == nullptr) {
    dependencies_.otaManager = std::make_shared<ota::OtaManager>();
  }
  if (dependencies_.observability == nullptr) {
    dependencies_.observability = std::make_shared<monitoring::ObservabilityRegistry>();
  }
}

CloudPlatform::~CloudPlatform() noexcept { Stop(); }

bool CloudPlatform::Start() {
  std::lock_guard lock{mutex_};
  if (running_) {
    return false;
  }
  running_ = true;
  return true;
}

void CloudPlatform::Stop() noexcept {
  std::lock_guard lock{mutex_};
  running_ = false;
}

CloudPlatformStatus CloudPlatform::Status() const {
  std::lock_guard lock{mutex_};
  CloudPlatformStatus status{};
  status.running = running_;
  status.restEndpointCount = dependencies_.restApiCatalog->Endpoints().size();
  status.robotCount = dependencies_.fleetManager->Status().robotCount;
  return status;
}

std::shared_ptr<api::RestApiCatalog> CloudPlatform::RestApi() const noexcept {
  return dependencies_.restApiCatalog;
}

std::shared_ptr<fleet::FleetManager> CloudPlatform::Fleet() const noexcept {
  return dependencies_.fleetManager;
}

std::shared_ptr<auth::AuthManager> CloudPlatform::Auth() const noexcept {
  return dependencies_.authManager;
}

std::shared_ptr<ota::OtaManager> CloudPlatform::Ota() const noexcept {
  return dependencies_.otaManager;
}

std::shared_ptr<monitoring::ObservabilityRegistry> CloudPlatform::Observability() const noexcept {
  return dependencies_.observability;
}

} // namespace humanoid::cloud
