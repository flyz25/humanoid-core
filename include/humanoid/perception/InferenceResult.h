#pragma once

/**
 * @file InferenceResult.h
 * @brief Defines generic inference result values.
 */

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/DetectionResult.h>

namespace humanoid::perception {

/**
 * @brief Supported scalar inference output value.
 */
using InferenceOutputValue =
    std::variant<bool, std::int64_t, double, std::string, std::vector<double>>;

/**
 * @brief Ordered inference output map.
 */
using InferenceOutputs = std::map<std::string, InferenceOutputValue, std::less<>>;

/**
 * @brief Vendor-independent inference result.
 */
struct InferenceResult final {
  /** @brief Operation status. */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /** @brief Model identifier that produced the result. */
  std::string modelId;

  /** @brief Backend-independent named outputs. */
  InferenceOutputs outputs;

  /** @brief Optional detections produced directly by the engine. */
  DetectionResults detections;

  /** @brief Monotonic result timestamp. */
  std::chrono::steady_clock::time_point timestamp{};

  /**
   * @brief Reports whether inference succeeded.
   *
   * @return True when status is OK.
   */
  [[nodiscard]] bool succeeded() const noexcept { return status.isOk(); }
};

} // namespace humanoid::perception
