#pragma once

/**
 * @file PerceptionPipeline.h
 * @brief Defines the generic perception pipeline graph.
 */

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/DetectionResult.h>
#include <humanoid/perception/InferenceResult.h>
#include <humanoid/perception/SensorFrame.h>

namespace humanoid::perception {

/**
 * @brief Standard perception pipeline stage category.
 */
enum class PerceptionStageType : std::uint8_t {
  Frame,     ///< Frame ingress or normalization.
  Filter,    ///< Frame filtering or preprocessing.
  Inference, ///< Inference execution.
  Detection, ///< Detection post-processing.
  Tracking,  ///< Tracking association.
  Output,    ///< Output routing or publication.
  Custom     ///< Application-defined stage.
};

/**
 * @brief Scalar stage configuration value.
 */
using PerceptionStageConfigValue = std::variant<bool, std::int64_t, double, std::string>;

/**
 * @brief Ordered stage configuration map.
 */
using PerceptionStageConfig = std::map<std::string, PerceptionStageConfigValue, std::less<>>;

/**
 * @brief Pipeline execution context passed between stages.
 */
struct PerceptionContext final {
  /** @brief Current frame being processed. */
  SensorFrame frame;

  /** @brief Inference results accumulated by stages. */
  std::vector<InferenceResult> inferenceResults;

  /** @brief Detection results accumulated by stages. */
  DetectionResults detections;

  /** @brief Generic stage outputs and metadata. */
  SensorFrameMetadata metadata;

  /** @brief Monotonic timestamp for this pipeline execution. */
  std::chrono::steady_clock::time_point timestamp{};
};

/**
 * @brief Stage declaration inside the pipeline graph.
 */
struct PerceptionStageDescriptor final {
  /** @brief Stable stage identifier. */
  std::string stageId;

  /** @brief Human-readable stage name. */
  std::string name;

  /** @brief Stage category. */
  PerceptionStageType type{PerceptionStageType::Custom};

  /** @brief Whether the stage is executed. */
  bool enabled{true};

  /** @brief Stage ids that must run before this stage. */
  std::vector<std::string> dependencies;

  /** @brief Backend-independent stage configuration. */
  PerceptionStageConfig configuration;

  /**
   * @brief Reports whether the descriptor has a usable identity.
   *
   * @return True when stageId and name are not empty.
   */
  [[nodiscard]] bool isValid() const noexcept { return !stageId.empty() && !name.empty(); }
};

/**
 * @brief Pure abstract perception pipeline stage.
 */
class IPerceptionStage {
public:
  /** @brief Destroys the stage interface. */
  virtual ~IPerceptionStage() = default;

  /**
   * @brief Processes the pipeline context.
   *
   * Implementations must contain backend exceptions and return status values.
   *
   * @param context Mutable pipeline context.
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Process(PerceptionContext& context) = 0;
};

/**
 * @brief Result returned by one pipeline execution.
 */
struct PerceptionPipelineResult final {
  /** @brief Pipeline execution status. */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /** @brief Final pipeline context. */
  PerceptionContext context;

  /** @brief Ordered stage ids that were executed. */
  std::vector<std::string> executedStages;

  /**
   * @brief Reports whether execution succeeded.
   *
   * @return True when status is OK.
   */
  [[nodiscard]] bool succeeded() const noexcept { return status.isOk(); }
};

/**
 * @brief Thread-safe dependency-injected perception pipeline graph.
 *
 * The graph owns no sensors, inference engines, trackers, or SDK clients.
 * Stages are injected as interfaces and executed according to declared
 * dependencies. Execution is serialized per pipeline instance so injected
 * stages do not need to be reentrant unless shared across pipelines.
 */
class PerceptionPipeline final {
public:
  /** @brief Constructs an empty pipeline. */
  PerceptionPipeline();

  /** @brief Destroys registered stage references. */
  ~PerceptionPipeline() noexcept;

  PerceptionPipeline(const PerceptionPipeline&) = delete;
  PerceptionPipeline& operator=(const PerceptionPipeline&) = delete;
  PerceptionPipeline(PerceptionPipeline&&) = delete;
  PerceptionPipeline& operator=(PerceptionPipeline&&) = delete;

  /**
   * @brief Registers a stage in the graph.
   *
   * @param descriptor Stage descriptor.
   * @param stage Stage implementation.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status RegisterStage(PerceptionStageDescriptor descriptor,
                                                       std::shared_ptr<IPerceptionStage> stage);

  /**
   * @brief Removes a stage from the graph.
   *
   * @param stage_id Stage id.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status UnregisterStage(std::string_view stage_id);

  /**
   * @brief Replaces a registered stage configuration.
   *
   * @param stage_id Stage id.
   * @param configuration Stage configuration.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status ConfigureStage(std::string_view stage_id,
                                                        PerceptionStageConfig configuration);

  /**
   * @brief Enables or disables a registered stage.
   *
   * @param stage_id Stage id.
   * @param enabled Desired enabled state.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status SetStageEnabled(std::string_view stage_id, bool enabled);

  /**
   * @brief Removes every stage from the graph.
   */
  void Clear();

  /**
   * @brief Returns registered stage descriptors ordered by stage id.
   *
   * @return Stage descriptors.
   */
  [[nodiscard]] std::vector<PerceptionStageDescriptor> DescribeGraph() const;

  /**
   * @brief Executes enabled stages for one frame.
   *
   * @param frame Input frame.
   * @return Pipeline result.
   */
  [[nodiscard]] PerceptionPipelineResult Execute(SensorFrame frame);

  /**
   * @brief Returns the number of registered stages.
   *
   * @return Stage count.
   */
  [[nodiscard]] std::size_t Size() const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::perception
