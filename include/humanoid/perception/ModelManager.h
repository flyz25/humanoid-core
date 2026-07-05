#pragma once

/**
 * @file ModelManager.h
 * @brief Defines a thread-safe inference model and engine registry.
 */

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/IInferenceEngine.h>

namespace humanoid::perception {

/**
 * @brief Backend-independent model metadata.
 */
struct ModelDescriptor final {
  /** @brief Stable model identifier. */
  std::string modelId;

  /** @brief Human-readable model name. */
  std::string name;

  /** @brief Model format label, for example "onnx" or "custom". */
  std::string format;

  /** @brief Engine id expected to run this model. */
  std::string engineId;

  /** @brief Optional semantic version or model revision. */
  std::string version;

  /** @brief Registration timestamp. */
  std::chrono::steady_clock::time_point timestamp{};

  /**
   * @brief Reports whether the descriptor has required identity fields.
   *
   * @return True when modelId, name, format, and engineId are not empty.
   */
  [[nodiscard]] bool isValid() const noexcept {
    return !modelId.empty() && !name.empty() && !format.empty() && !engineId.empty();
  }
};

/**
 * @brief Result returned by model lookup operations.
 */
struct ModelDescriptorResult final {
  /** @brief Operation status. */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /** @brief Model descriptor on success. */
  ModelDescriptor descriptor;

  /**
   * @brief Reports whether lookup succeeded.
   *
   * @return True when status is OK.
   */
  [[nodiscard]] bool succeeded() const noexcept { return status.isOk(); }
};

/**
 * @brief Thread-safe registry for inference engines and model metadata.
 *
 * `ModelManager` owns injected engine interfaces and model descriptors. It
 * performs no model parsing, backend initialization policy, filesystem loading,
 * network download, or inference by itself.
 */
class ModelManager final {
public:
  /** @brief Constructs an empty model manager. */
  ModelManager();

  /** @brief Destroys registered engine interfaces. */
  ~ModelManager() noexcept;

  ModelManager(const ModelManager&) = delete;
  ModelManager& operator=(const ModelManager&) = delete;
  ModelManager(ModelManager&&) = delete;
  ModelManager& operator=(ModelManager&&) = delete;

  /**
   * @brief Registers an inference engine.
   *
   * @param engine Engine instance.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status RegisterEngine(std::shared_ptr<IInferenceEngine> engine);

  /**
   * @brief Removes a registered inference engine.
   *
   * @param engine_id Engine id.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status UnregisterEngine(std::string_view engine_id);

  /**
   * @brief Registers model metadata.
   *
   * @param descriptor Model descriptor.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status RegisterModel(ModelDescriptor descriptor);

  /**
   * @brief Removes model metadata.
   *
   * @param model_id Model id.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status UnregisterModel(std::string_view model_id);

  /**
   * @brief Looks up a model descriptor.
   *
   * @param model_id Model id.
   * @return Descriptor result.
   */
  [[nodiscard]] ModelDescriptorResult FindModel(std::string_view model_id) const;

  /**
   * @brief Enumerates registered models.
   *
   * @return Model descriptors ordered by model id.
   */
  [[nodiscard]] std::vector<ModelDescriptor> EnumerateModels() const;

  /**
   * @brief Enumerates registered inference engine capabilities.
   *
   * @return Capability declarations ordered by engine id.
   */
  [[nodiscard]] std::vector<InferenceEngineCapabilities> EnumerateEngines() const;

  /**
   * @brief Runs inference for a registered model through its registered engine.
   *
   * @param request Inference request.
   * @return Inference result.
   */
  [[nodiscard]] InferenceResult RunInference(const InferenceRequest& request);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::perception
