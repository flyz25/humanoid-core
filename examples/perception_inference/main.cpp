/**
 * @file main.cpp
 * @brief Demonstrates inference engine dependency injection.
 */

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <humanoid/perception/IInferenceEngine.h>
#include <humanoid/perception/InferenceRequest.h>
#include <humanoid/perception/InferenceResult.h>
#include <humanoid/perception/ModelManager.h>
#include <humanoid/perception/SensorFrame.h>
#include <humanoid/perception/SensorType.h>

namespace {

class ExampleEngine final : public humanoid::perception::IInferenceEngine {
public:
  [[nodiscard]] humanoid::common::Status Initialize() override {
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status Shutdown() override {
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::perception::InferenceResult
  Run(const humanoid::perception::InferenceRequest& request) override {
    humanoid::perception::InferenceResult result;
    result.modelId = request.modelId;
    result.outputs.emplace("confidence", 0.88);
    result.timestamp = std::chrono::steady_clock::now();
    return result;
  }

  [[nodiscard]] humanoid::perception::InferenceEngineCapabilities Capabilities() const override {
    humanoid::perception::InferenceEngineCapabilities capabilities;
    capabilities.engineId = "example";
    capabilities.name = "Example Engine";
    capabilities.supportedBackends = {"Custom"};
    capabilities.supportedModelFormats = {"memory"};
    return capabilities;
  }
};

} // namespace

int main() {
  humanoid::perception::ModelManager manager;
  if (!manager.RegisterEngine(std::make_shared<ExampleEngine>()).isOk()) {
    return 1;
  }

  humanoid::perception::ModelDescriptor descriptor;
  descriptor.modelId = "example-model";
  descriptor.name = "Example Model";
  descriptor.format = "memory";
  descriptor.engineId = "example";
  if (!manager.RegisterModel(descriptor).isOk()) {
    return 1;
  }

  humanoid::perception::InferenceRequest request;
  request.modelId = "example-model";
  request.inputFrame.id = 1U;
  request.inputFrame.sensorType = humanoid::perception::SensorType::Camera;
  request.inputFrame.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  request.inputFrame.data = std::vector<std::byte>{std::byte{1}, std::byte{2}};

  const humanoid::perception::InferenceResult result = manager.RunInference(request);
  if (!result.succeeded()) {
    return 1;
  }

  std::cout << "Inference model=" << result.modelId << " outputs=" << result.outputs.size() << '\n';
  return 0;
}
