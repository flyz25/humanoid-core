#include <humanoid/core/CoreContext.hpp>

#include <utility>

namespace humanoid::core {

CoreContext::CoreContext(std::shared_ptr<logging::ILogger> logger,
                         std::shared_ptr<const configuration::Configuration> configuration)
    : logger_(std::move(logger)), configuration_(std::move(configuration)) {}

void CoreContext::setLogger(std::shared_ptr<logging::ILogger> logger) noexcept {
  logger_ = std::move(logger);
}

void CoreContext::setConfiguration(
    std::shared_ptr<const configuration::Configuration> configuration) noexcept {
  configuration_ = std::move(configuration);
}

bool CoreContext::hasLogger() const noexcept { return static_cast<bool>(logger_); }

bool CoreContext::hasConfiguration() const noexcept { return static_cast<bool>(configuration_); }

std::shared_ptr<logging::ILogger> CoreContext::logger() const noexcept { return logger_; }

std::shared_ptr<const configuration::Configuration> CoreContext::configuration() const noexcept {
  return configuration_;
}

} // namespace humanoid::core
