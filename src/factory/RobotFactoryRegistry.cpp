#include <factory/RobotFactoryRegistry.h>

#include <algorithm>
#include <utility>

namespace humanoid::factory {
namespace {

/**
 * @brief Creates a successful result.
 *
 * @param message Diagnostic message.
 * @return Successful result.
 */
adapters::Result Success(std::string message) {
  return adapters::Result{adapters::ErrorCode::kSuccess, std::move(message)};
}

/**
 * @brief Creates an error result.
 *
 * @param code Error code.
 * @param message Diagnostic message.
 * @return Error result.
 */
adapters::Result Failure(adapters::ErrorCode code, std::string message) {
  return adapters::Result{code, std::move(message)};
}

} // namespace

adapters::Result
RobotFactoryRegistry::RegisterFactory(std::shared_ptr<adapters::IRobotFactory> factory) {
  if (!factory) {
    return Failure(adapters::ErrorCode::kUnknown, "robot factory is null");
  }

  std::lock_guard<std::mutex> lock{mutex_};
  const auto duplicate =
      std::find_if(factories_.begin(), factories_.end(),
                   [&factory](const std::shared_ptr<adapters::IRobotFactory>& registered) {
                     if (!registered) {
                       return false;
                     }

                     for (const std::string& model : factory->SupportedModels()) {
                       if (registered->Supports(factory->Vendor(), model)) {
                         return true;
                       }
                     }

                     return false;
                   });

  if (duplicate != factories_.end()) {
    return Failure(adapters::ErrorCode::kUnknown, "robot factory is already registered");
  }

  factories_.push_back(std::move(factory));
  return Success("robot factory registered");
}

std::shared_ptr<adapters::IRobotFactory>
RobotFactoryRegistry::FindFactory(std::string_view vendor, std::string_view model) const {
  std::lock_guard<std::mutex> lock{mutex_};
  const auto match =
      std::find_if(factories_.begin(), factories_.end(),
                   [vendor, model](const std::shared_ptr<adapters::IRobotFactory>& factory) {
                     return factory && factory->Supports(vendor, model);
                   });

  return match == factories_.end() ? nullptr : *match;
}

std::unique_ptr<adapters::IRobotAdapter>
RobotFactoryRegistry::CreateAdapter(const adapters::RobotConfig& config,
                                    std::shared_ptr<logging::ILogger> logger) const {
  const std::shared_ptr<adapters::IRobotFactory> factory = FindFactory(config.vendor, config.model);
  if (!factory) {
    return nullptr;
  }

  return factory->CreateAdapter(config, std::move(logger));
}

std::size_t RobotFactoryRegistry::FactoryCount() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return factories_.size();
}

std::vector<std::string> RobotFactoryRegistry::RegisteredVendors() const {
  std::lock_guard<std::mutex> lock{mutex_};
  std::vector<std::string> vendors;
  vendors.reserve(factories_.size());

  for (const std::shared_ptr<adapters::IRobotFactory>& factory : factories_) {
    if (factory) {
      vendors.emplace_back(factory->Vendor());
    }
  }

  return vendors;
}

} // namespace humanoid::factory
