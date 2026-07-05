#pragma once

/**
 * @file InferenceRequest.h
 * @brief Defines generic inference request values.
 */

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <variant>

#include <humanoid/perception/SensorFrame.h>

namespace humanoid::perception {

/**
 * @brief Supported scalar inference parameter value.
 */
using InferenceParameterValue = std::variant<bool, std::int64_t, double, std::string>;

/**
 * @brief Ordered inference parameter map.
 */
using InferenceParameters = std::map<std::string, InferenceParameterValue, std::less<>>;

/**
 * @brief Vendor-independent inference request.
 */
struct InferenceRequest final {
  /** @brief Target model identifier. */
  std::string modelId;

  /** @brief Input frame for the inference request. */
  SensorFrame inputFrame;

  /** @brief Backend-independent scalar parameters. */
  InferenceParameters parameters;

  /** @brief Monotonic request timestamp. */
  std::chrono::steady_clock::time_point timestamp{};

  /**
   * @brief Reports whether the request references a model.
   *
   * @return True when modelId is not empty.
   */
  [[nodiscard]] bool hasModel() const noexcept { return !modelId.empty(); }
};

} // namespace humanoid::perception
