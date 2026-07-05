#pragma once

/**
 * @file IInferenceEngine.h
 * @brief Defines the generic inference engine boundary.
 */

#include <string>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/InferenceRequest.h>
#include <humanoid/perception/InferenceResult.h>

namespace humanoid::perception {

/**
 * @brief Static capability declaration for an inference engine.
 */
struct InferenceEngineCapabilities final {
  /** @brief Engine identifier. */
  std::string engineId;

  /** @brief Human-readable engine name. */
  std::string name;

  /** @brief Supported backend family labels such as "TensorRT" or "Custom". */
  std::vector<std::string> supportedBackends;

  /** @brief Supported model format labels such as "onnx" or "engine". */
  std::vector<std::string> supportedModelFormats;

  /**
   * @brief Reports whether the capability declaration has a usable identity.
   *
   * @return True when engineId and name are not empty.
   */
  [[nodiscard]] bool isValid() const noexcept { return !engineId.empty() && !name.empty(); }
};

/**
 * @brief Pure abstract inference engine interface.
 *
 * Implementations may wrap TensorRT, ONNX Runtime, Torch, OpenVINO, local
 * custom engines, or remote engines. No backend headers or backend types may
 * cross this boundary.
 */
class IInferenceEngine {
public:
  /** @brief Destroys the inference engine interface. */
  virtual ~IInferenceEngine() = default;

  /**
   * @brief Initializes backend resources.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Initialize() = 0;

  /**
   * @brief Releases backend resources.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Shutdown() = 0;

  /**
   * @brief Executes one inference request.
   *
   * Implementations must translate backend failures into `InferenceResult` and
   * contain backend exceptions.
   *
   * @param request Inference request.
   * @return Inference result.
   */
  [[nodiscard]] virtual InferenceResult Run(const InferenceRequest& request) = 0;

  /**
   * @brief Returns static engine capabilities.
   *
   * @return Capability declaration.
   */
  [[nodiscard]] virtual InferenceEngineCapabilities Capabilities() const = 0;
};

} // namespace humanoid::perception
