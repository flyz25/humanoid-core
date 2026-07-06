#include <humanoid/cloud/api/RestApiCatalog.h>

#include <cstdlib>
#include <iostream>

int main() {
  humanoid::cloud::api::RestApiCatalog catalog;
  const auto robot_endpoints = catalog.FindBySubsystem("Robot");

  if (robot_endpoints.empty()) {
    return EXIT_FAILURE;
  }

  std::cout << "REST API endpoints: " << catalog.Endpoints().size()
            << ", robot endpoints: " << robot_endpoints.size() << '\n';
  return EXIT_SUCCESS;
}
