#include <humanoid/planner/PlanningPipeline.h>

#include <chrono>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace humanoid::planner {

namespace {

[[nodiscard]] humanoid::common::Status InvalidArgument(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kInvalidArgument,
                                         std::move(message));
}

[[nodiscard]] humanoid::common::Status FailedPrecondition(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kFailedPrecondition,
                                         std::move(message));
}

[[nodiscard]] humanoid::common::Status InternalError(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kInternalError,
                                         std::move(message));
}

void AddDiagnostic(PlanningResult& result, PlanningDiagnosticSeverity severity, std::string code,
                   std::string message) {
  result.diagnostics.push_back(PlanningDiagnostic{severity, std::move(code), std::move(message)});
}

[[nodiscard]] bool IsPlanAccepted(const PlanningResult& result) noexcept {
  return result.status == GoalStatus::Planned && result.hasMission() && result.hasBehaviorTree();
}

} // namespace

PlanningPipeline::PlanningPipeline(
    std::shared_ptr<IPlanner> planner, std::shared_ptr<IPlanner> fallback_planner,
    std::shared_ptr<humanoid::bt::BehaviorTreeRuntime> behavior_tree_runtime,
    std::shared_ptr<humanoid::logging::ILogger> logger)
    : planner_(std::move(planner)), fallback_planner_(std::move(fallback_planner)),
      behavior_tree_runtime_(std::move(behavior_tree_runtime)), logger_(std::move(logger)) {
  if (!planner_) {
    throw std::invalid_argument{"PlanningPipeline requires a primary planner"};
  }
}

PlanningPipelineResult PlanningPipeline::Execute(const PlanningRequest& request,
                                                 PlanningPipelineOptions options) {
  std::lock_guard<std::mutex> execution_lock{execution_mutex_};

  {
    std::lock_guard<std::mutex> lock{metrics_mutex_};
    ++metrics_.requests;
  }

  Log(humanoid::logging::LogLevel::kInfo, "planning pipeline started");

  PlanningPipelineResult pipeline_result;
  if (!request.isValid()) {
    pipeline_result.status = InvalidArgument("planning request is invalid");
    pipeline_result.planningResult.status = GoalStatus::Rejected;
    AddDiagnostic(pipeline_result.planningResult, PlanningDiagnosticSeverity::Error,
                  "pipeline.invalid_request", "planning request is invalid");
    IncrementRejected();
    Log(humanoid::logging::LogLevel::kWarning, "planning request rejected");
    return pipeline_result;
  }

  auto run_planner = [this, &request, &options](const std::shared_ptr<IPlanner>& planner,
                                                bool fallback) -> PlanningPipelineResult {
    PlanningPipelineResult result;
    if (!planner) {
      result.status = FailedPrecondition("planner dependency is unavailable");
      result.planningResult.status = GoalStatus::Failed;
      AddDiagnostic(result.planningResult, PlanningDiagnosticSeverity::Error,
                    fallback ? "pipeline.fallback_unavailable" : "pipeline.primary_unavailable",
                    "planner dependency is unavailable");
      return result;
    }

    {
      std::lock_guard<std::mutex> lock{metrics_mutex_};
      if (fallback) {
        ++metrics_.fallbackPlannerAttempts;
      } else {
        ++metrics_.primaryPlannerAttempts;
      }
    }

    try {
      result.planningResult = planner->Plan(request);
    } catch (const std::exception& exception) {
      result.status = InternalError(std::string{"planner threw exception: "} + exception.what());
      result.planningResult.status = GoalStatus::Failed;
      AddDiagnostic(result.planningResult, PlanningDiagnosticSeverity::Error,
                    fallback ? "pipeline.fallback_exception" : "pipeline.primary_exception",
                    exception.what());
      return result;
    } catch (...) {
      result.status = InternalError("planner threw unknown exception");
      result.planningResult.status = GoalStatus::Failed;
      AddDiagnostic(result.planningResult, PlanningDiagnosticSeverity::Error,
                    fallback ? "pipeline.fallback_exception" : "pipeline.primary_exception",
                    "planner threw unknown exception");
      return result;
    }

    if (!IsPlanAccepted(result.planningResult)) {
      result.status =
          FailedPrecondition("planner did not produce mission and behavior tree output");
      AddDiagnostic(result.planningResult, PlanningDiagnosticSeverity::Warning,
                    fallback ? "pipeline.fallback_not_accepted" : "pipeline.primary_not_accepted",
                    "planner did not produce mission and behavior tree output");
      return result;
    }

    if (options.validatePlan) {
      const humanoid::common::Status validation_status =
          planner->ValidatePlan(result.planningResult);
      if (!validation_status.isOk()) {
        result.status = validation_status;
        AddDiagnostic(result.planningResult, PlanningDiagnosticSeverity::Error,
                      fallback ? "pipeline.fallback_validation_failed"
                               : "pipeline.primary_validation_failed",
                      validation_status.message());
        {
          std::lock_guard<std::mutex> lock{metrics_mutex_};
          ++metrics_.validationFailures;
        }
        return result;
      }
    }

    result.status = humanoid::common::Status::ok();
    result.usedFallback = fallback;
    return result;
  };

  pipeline_result = run_planner(planner_, false);
  if (!pipeline_result.status.isOk() && options.enableFallback && fallback_planner_) {
    Log(humanoid::logging::LogLevel::kWarning,
        "primary planner failed; attempting fallback planner");
    PlanningPipelineResult fallback_result = run_planner(fallback_planner_, true);
    if (fallback_result.status.isOk()) {
      pipeline_result = std::move(fallback_result);
      {
        std::lock_guard<std::mutex> lock{metrics_mutex_};
        ++metrics_.fallbackSucceeded;
      }
    }
  }

  if (!pipeline_result.status.isOk()) {
    IncrementFailure();
    Log(humanoid::logging::LogLevel::kError, "planning pipeline failed");
    return pipeline_result;
  }

  if (options.submitBehaviorTreeToRuntime) {
    if (!behavior_tree_runtime_) {
      pipeline_result.status = FailedPrecondition("behavior tree runtime is unavailable");
      AddDiagnostic(pipeline_result.planningResult, PlanningDiagnosticSeverity::Error,
                    "pipeline.runtime_unavailable",
                    "behavior tree runtime is unavailable for execution handoff");
      {
        std::lock_guard<std::mutex> lock{metrics_mutex_};
        ++metrics_.runtimeSubmissionFailures;
      }
      IncrementFailure();
      Log(humanoid::logging::LogLevel::kError, "runtime handoff failed");
      return pipeline_result;
    }

    if (!pipeline_result.planningResult.hasBehaviorTree()) {
      pipeline_result.status = FailedPrecondition("planning result has no behavior tree");
      AddDiagnostic(pipeline_result.planningResult, PlanningDiagnosticSeverity::Error,
                    "pipeline.runtime_missing_tree",
                    "planning result has no behavior tree for execution handoff");
      {
        std::lock_guard<std::mutex> lock{metrics_mutex_};
        ++metrics_.runtimeSubmissionFailures;
      }
      IncrementFailure();
      return pipeline_result;
    }

    try {
      pipeline_result.runtimeHandle =
          behavior_tree_runtime_->Submit(std::move(pipeline_result.planningResult.behaviorTree),
                                         std::move(options.runtimeOptions));
      {
        std::lock_guard<std::mutex> lock{metrics_mutex_};
        ++metrics_.runtimeSubmissions;
      }
      Log(humanoid::logging::LogLevel::kInfo, "planning pipeline submitted behavior tree");
    } catch (const std::exception& exception) {
      pipeline_result.status =
          InternalError(std::string{"runtime handoff threw exception: "} + exception.what());
      AddDiagnostic(pipeline_result.planningResult, PlanningDiagnosticSeverity::Error,
                    "pipeline.runtime_exception", exception.what());
      {
        std::lock_guard<std::mutex> lock{metrics_mutex_};
        ++metrics_.runtimeSubmissionFailures;
      }
      IncrementFailure();
      return pipeline_result;
    } catch (...) {
      pipeline_result.status = InternalError("runtime handoff threw unknown exception");
      AddDiagnostic(pipeline_result.planningResult, PlanningDiagnosticSeverity::Error,
                    "pipeline.runtime_exception", "runtime handoff threw unknown exception");
      {
        std::lock_guard<std::mutex> lock{metrics_mutex_};
        ++metrics_.runtimeSubmissionFailures;
      }
      IncrementFailure();
      return pipeline_result;
    }
  }

  {
    std::lock_guard<std::mutex> lock{metrics_mutex_};
    ++metrics_.planned;
  }
  Log(humanoid::logging::LogLevel::kInfo, "planning pipeline completed");
  return pipeline_result;
}

PlanningPipelineMetrics PlanningPipeline::GetMetrics() const {
  std::lock_guard<std::mutex> lock{metrics_mutex_};
  return metrics_;
}

void PlanningPipeline::Log(humanoid::logging::LogLevel level, const std::string& message) const {
  if (!logger_ || !logger_->isEnabled(level)) {
    return;
  }

  (void)logger_->log(humanoid::logging::LogMessage{level, "PlanningPipeline", message});
}

void PlanningPipeline::IncrementRejected() {
  std::lock_guard<std::mutex> lock{metrics_mutex_};
  ++metrics_.rejected;
}

void PlanningPipeline::IncrementFailure() {
  std::lock_guard<std::mutex> lock{metrics_mutex_};
  ++metrics_.failed;
}

} // namespace humanoid::planner
