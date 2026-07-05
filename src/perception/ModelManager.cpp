#include <humanoid/perception/ModelManager.h>

#include <chrono>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>

namespace humanoid::perception {
namespace {

[[nodiscard]] humanoid::common::Status Error(humanoid::common::StatusCode code,
                                             std::string message) {
  return humanoid::common::Status::error(code, std::move(message));
}

[[nodiscard]] std::chrono::steady_clock::time_point Now() noexcept {
  return std::chrono::steady_clock::now();
}

} // namespace

class ModelManager::Impl final {
public:
  Impl() = default;

  [[nodiscard]] humanoid::common::Status RegisterEngine(std::shared_ptr<IInferenceEngine> engine) {
    if (!engine) {
      return Error(humanoid::common::StatusCode::kInvalidArgument, "inference engine is null");
    }

    InferenceEngineCapabilities capabilities;
    try {
      capabilities = engine->Capabilities();
    } catch (...) {
      return Error(humanoid::common::StatusCode::kInternalError,
                   "inference engine capabilities query threw an exception");
    }

    if (!capabilities.isValid()) {
      return Error(humanoid::common::StatusCode::kInvalidArgument,
                   "inference engine requires non-empty id and name");
    }

    std::string engine_id = capabilities.engineId;
    std::unique_lock<std::shared_mutex> lock{mutex_};
    const auto [iterator, inserted] = engines_.emplace(engine_id, std::move(engine));
    static_cast<void>(iterator);
    if (!inserted) {
      return Error(humanoid::common::StatusCode::kFailedPrecondition,
                   "inference engine id is already registered");
    }
    engine_capabilities_.emplace(std::move(engine_id), std::move(capabilities));
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status UnregisterEngine(std::string_view engine_id) {
    if (engine_id.empty()) {
      return Error(humanoid::common::StatusCode::kInvalidArgument, "engine id is empty");
    }

    std::unique_lock<std::shared_mutex> lock{mutex_};
    const std::string key{engine_id};
    if (engines_.erase(key) == 0U) {
      return Error(humanoid::common::StatusCode::kUnavailable,
                   "inference engine id is not registered");
    }
    engine_capabilities_.erase(key);
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status RegisterModel(ModelDescriptor descriptor) {
    if (!descriptor.isValid()) {
      return Error(humanoid::common::StatusCode::kInvalidArgument,
                   "model requires non-empty id, name, format, and engine id");
    }
    if (descriptor.timestamp == std::chrono::steady_clock::time_point{}) {
      descriptor.timestamp = Now();
    }

    std::unique_lock<std::shared_mutex> lock{mutex_};
    if (!engines_.contains(descriptor.engineId)) {
      return Error(humanoid::common::StatusCode::kFailedPrecondition,
                   "model references an unregistered inference engine");
    }

    std::string model_id = descriptor.modelId;
    const auto [iterator, inserted] = models_.emplace(std::move(model_id), std::move(descriptor));
    static_cast<void>(iterator);
    if (!inserted) {
      return Error(humanoid::common::StatusCode::kFailedPrecondition,
                   "model id is already registered");
    }
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status UnregisterModel(std::string_view model_id) {
    if (model_id.empty()) {
      return Error(humanoid::common::StatusCode::kInvalidArgument, "model id is empty");
    }

    std::unique_lock<std::shared_mutex> lock{mutex_};
    if (models_.erase(std::string{model_id}) == 0U) {
      return Error(humanoid::common::StatusCode::kUnavailable, "model id is not registered");
    }
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] ModelDescriptorResult FindModel(std::string_view model_id) const {
    if (model_id.empty()) {
      return ModelDescriptorResult{
          Error(humanoid::common::StatusCode::kInvalidArgument, "model id is empty"), {}};
    }

    std::shared_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = models_.find(std::string{model_id});
    if (iterator == models_.end()) {
      return ModelDescriptorResult{
          Error(humanoid::common::StatusCode::kUnavailable, "model id is not registered"), {}};
    }
    return ModelDescriptorResult{humanoid::common::Status::ok(), iterator->second};
  }

  [[nodiscard]] std::vector<ModelDescriptor> EnumerateModels() const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    std::vector<ModelDescriptor> models;
    models.reserve(models_.size());
    for (const auto& [model_id, descriptor] : models_) {
      static_cast<void>(model_id);
      models.push_back(descriptor);
    }
    return models;
  }

  [[nodiscard]] std::vector<InferenceEngineCapabilities> EnumerateEngines() const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    std::vector<InferenceEngineCapabilities> capabilities;
    capabilities.reserve(engine_capabilities_.size());
    for (const auto& [engine_id, engine_capabilities] : engine_capabilities_) {
      static_cast<void>(engine_id);
      capabilities.push_back(engine_capabilities);
    }
    return capabilities;
  }

  [[nodiscard]] InferenceResult RunInference(const InferenceRequest& request) {
    if (!request.hasModel()) {
      InferenceResult result;
      result.status = Error(humanoid::common::StatusCode::kInvalidArgument,
                            "inference request model id is empty");
      return result;
    }

    std::shared_ptr<IInferenceEngine> engine;
    {
      std::shared_lock<std::shared_mutex> lock{mutex_};
      const auto model = models_.find(request.modelId);
      if (model == models_.end()) {
        InferenceResult result;
        result.status =
            Error(humanoid::common::StatusCode::kUnavailable, "model id is not registered");
        result.modelId = request.modelId;
        return result;
      }

      const auto engine_iterator = engines_.find(model->second.engineId);
      if (engine_iterator == engines_.end()) {
        InferenceResult result;
        result.status = Error(humanoid::common::StatusCode::kUnavailable,
                              "model inference engine is not registered");
        result.modelId = request.modelId;
        return result;
      }
      engine = engine_iterator->second;
    }

    try {
      InferenceResult result = engine->Run(request);
      if (result.timestamp == std::chrono::steady_clock::time_point{}) {
        result.timestamp = Now();
      }
      if (result.modelId.empty()) {
        result.modelId = request.modelId;
      }
      return result;
    } catch (...) {
      InferenceResult result;
      result.status = Error(humanoid::common::StatusCode::kInternalError,
                            "inference engine threw an exception");
      result.modelId = request.modelId;
      result.timestamp = Now();
      return result;
    }
  }

private:
  mutable std::shared_mutex mutex_;
  std::map<std::string, std::shared_ptr<IInferenceEngine>, std::less<>> engines_;
  std::map<std::string, InferenceEngineCapabilities, std::less<>> engine_capabilities_;
  std::map<std::string, ModelDescriptor, std::less<>> models_;
};

ModelManager::ModelManager() : impl_(std::make_unique<Impl>()) {}

ModelManager::~ModelManager() noexcept = default;

humanoid::common::Status ModelManager::RegisterEngine(std::shared_ptr<IInferenceEngine> engine) {
  return impl_->RegisterEngine(std::move(engine));
}

humanoid::common::Status ModelManager::UnregisterEngine(std::string_view engine_id) {
  return impl_->UnregisterEngine(engine_id);
}

humanoid::common::Status ModelManager::RegisterModel(ModelDescriptor descriptor) {
  return impl_->RegisterModel(std::move(descriptor));
}

humanoid::common::Status ModelManager::UnregisterModel(std::string_view model_id) {
  return impl_->UnregisterModel(model_id);
}

ModelDescriptorResult ModelManager::FindModel(std::string_view model_id) const {
  return impl_->FindModel(model_id);
}

std::vector<ModelDescriptor> ModelManager::EnumerateModels() const {
  return impl_->EnumerateModels();
}

std::vector<InferenceEngineCapabilities> ModelManager::EnumerateEngines() const {
  return impl_->EnumerateEngines();
}

InferenceResult ModelManager::RunInference(const InferenceRequest& request) {
  return impl_->RunInference(request);
}

} // namespace humanoid::perception
