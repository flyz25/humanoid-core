#include <cstdlib>
#include <iostream>

#include <humanoid/configuration/ConfigManager.hpp>
#include <humanoid/logging/LoggerManager.hpp>

int main() {
  const humanoid::logging::LoggerManager logger;
  const humanoid::configuration::ConfigManager config;

  (void)logger;
  (void)config;

  std::cout << "Humanoid Core Initialized" << '\n';
  return EXIT_SUCCESS;
}
