#include <humanoid/cloud/api/RestApiCatalog.h>

#include <cstdlib>
#include <iostream>

int main() {
  humanoid::cloud::api::RestApiCatalog catalog;
  const auto robot_endpoints = catalog.FindBySubsystem("Robot");

  for (const auto& endpoint : robot_endpoints) {
    if (endpoint.id == "robot.connect") {
      std::cout << "Remote control endpoint: " << endpoint.path << '\n';
      return EXIT_SUCCESS;
    }
  }

  return EXIT_FAILURE;
}
