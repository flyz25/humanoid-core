#include <humanoid/cloud/ota/OtaManager.h>

#include <cstdlib>
#include <iostream>

int main() {
  humanoid::cloud::ota::OtaManager ota;
  humanoid::cloud::ota::PackageDescriptor package{};
  package.packageId = "plugin-update";
  package.version = "1.2.3";
  package.type = humanoid::cloud::ota::PackageType::Plugin;
  package.verificationDigest = "sha256:example";
  package.rollbackSupported = true;

  if (!ota.RegisterPackage(package).ok()) {
    return EXIT_FAILURE;
  }
  if (!ota.PlanUpdate(package.packageId, {"robot-001"}).ok()) {
    return EXIT_FAILURE;
  }
  if (!ota.VerifyPackage(package.packageId, package.verificationDigest).ok()) {
    return EXIT_FAILURE;
  }

  std::cout << "OTA plans: " << ota.Plans().size() << '\n';
  return EXIT_SUCCESS;
}
