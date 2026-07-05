#pragma once

/**
 * @file PlanningPipeline.h
 * @brief Defines the vendor-independent planning orchestration pipeline.
 */

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <humanoid/bt/BehaviorTreeRuntime.h>
#include <humanoid/common/Status.hpp>
#include <humanoid/logging/Logger.hpp>
#include <humanoid/planner/IPlanner.h>
#include <humanoid/planner/PlanningRequest.h>
#include <humanoid/planner/PlanningResult.h>
#include <humanoid/runtime/RuntimeScheduler.h>

namespace humanoid::planner {

/**
 * @brief Options for one planning pipeline execution.
 */
struct PlanningPipelineOptions final {
  /**
   * @brief True when `IPlanner::ValidatePlan()` should gate success.
   */
  bool validatePlan{true};

  /**
   * @brief True when the fallback planner may be used after primary failure.
   */
  bool enableFallback{true};

  /**
   * @brief True when a generated behavior tree should be submitted to runtime.
   */
  bool submitBehaviorTreeToRuntime{false};

  /**
   * @brief Runtime submission options used when execution handoff is enabled.
   */
  humanoid::bt::BehaviorTreeJobOptions runtimeOptions;
};

/**
 * @brief Metrics snapshot for planning pipeline activity.
 */
struct PlanningPipelineMetrics final {
  /** @brief Total pipeline execution requests accepted by the object. */
  std::uint64_t requests{0U};

  /** @brief Requests rejected before planner invocation. */
  std::uint64_t rejected{0U};

  /** @brief Primary planner invocations. */
  std::uint64_t primaryPlannerAttempts{0U};

  /** @brief Fallback planner invocations. */
  std::uint64_t fallbackPlannerAttempts{0U};

  /** @brief Plans accepted after validation policy. */
  std::uint64_t planned{0U};

  /** @brief Plans accepted from the fallback planner. */
  std::uint64_t fallbackSucceeded{0U};

  /** @brief Plan validation failures. */
  std::uint64_t validationFailures{0U};

  /** @brief Pipeline executions that ended in failure. */
  std::uint64_t failed{0U};

  /** @brief Behavior tree runtime handoffs attempted successfully. */
  std::uint64_t runtimeSubmissions{0U};

  /** @brief Runtime handoff attempts rejected before submission. */
  std::uint64_t runtimeSubmissionFailures{0U};
};

/**
 * @brief Result returned by `PlanningPipeline`.
 */
struct PlanningPipelineResult final {
  /**
   * @brief Pipeline operation status.
   */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /**
   * @brief Planner output. When runtime submission is enabled, the behavior tree
   * may have been moved into the runtime job while mission and diagnostics remain.
   */
  PlanningResult planningResult;

  /**
   * @brief True when the accepted plan came from the fallback planner.
   */
  bool usedFallback{false};

  /**
   * @brief Runtime job handle when behavior tree handoff was requested and accepted.
   */
  std::optional<humanoid::runtime::RuntimeJobHandle> runtimeHandle;
};

/**
 * @brief Thread-safe orchestration layer from goal planning to runtime handoff.
 *
 * `PlanningPipeline` composes injected planners, optional fallback planning,
 * validation, diagnostics, optional logging, metrics, and optional behavior
 * tree runtime submission. It contains no LLM provider implementation, no HTTP
 * client, no robot adapter, no SDK code, and no mission or behavior execution
 * policy of its own.
 */
class PlanningPipeline final {
public:
  /**
   * @brief Constructs a planning pipeline.
   *
   * @param planner Primary planner dependency.
   * @param fallback_planner Optional fallback planner dependency.
   * @param behavior_tree_runtime Optional runtime handoff dependency.
   * @param logger Optional logger dependency.
   * @throws std::invalid_argument when the primary planner is null.
   */
  explicit PlanningPipeline(
      std::shared_ptr<IPlanner> planner, std::shared_ptr<IPlanner> fallback_planner = {},
      std::shared_ptr<humanoid::bt::BehaviorTreeRuntime> behavior_tree_runtime = {},
      std::shared_ptr<humanoid::logging::ILogger> logger = {});

  /** @brief Destroys the planning pipeline. */
  ~PlanningPipeline() = default;

  PlanningPipeline(const PlanningPipeline&) = delete;
  PlanningPipeline& operator=(const PlanningPipeline&) = delete;
  PlanningPipeline(PlanningPipeline&&) = delete;
  PlanningPipeline& operator=(PlanningPipeline&&) = delete;

  /**
   * @brief Executes planning and optional runtime handoff.
   *
   * @param request Planning request.
   * @param options Pipeline execution options.
   * @return Pipeline result with plan diagnostics and optional runtime handle.
   */
  [[nodiscard]] PlanningPipelineResult Execute(const PlanningRequest& request,
                                               PlanningPipelineOptions options = {});

  /**
   * @brief Returns a consistent metrics snapshot.
   *
   * @return Pipeline metrics.
   */
  [[nodiscard]] PlanningPipelineMetrics GetMetrics() const;

private:
  void Log(humanoid::logging::LogLevel level, const std::string& message) const;
  void IncrementRejected();
  void IncrementFailure();

  std::shared_ptr<IPlanner> planner_;
  std::shared_ptr<IPlanner> fallback_planner_;
  std::shared_ptr<humanoid::bt::BehaviorTreeRuntime> behavior_tree_runtime_;
  std::shared_ptr<humanoid::logging::ILogger> logger_;
  mutable std::mutex execution_mutex_;
  mutable std::mutex metrics_mutex_;
  PlanningPipelineMetrics metrics_;
};

} // namespace humanoid::planner
