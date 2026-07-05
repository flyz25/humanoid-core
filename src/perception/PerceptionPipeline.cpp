#include <humanoid/perception/PerceptionPipeline.h>

#include <algorithm>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

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

class PerceptionPipeline::Impl final {
public:
  [[nodiscard]] humanoid::common::Status RegisterStage(PerceptionStageDescriptor descriptor,
                                                       std::shared_ptr<IPerceptionStage> stage) {
    if (!descriptor.isValid()) {
      return Error(humanoid::common::StatusCode::kInvalidArgument,
                   "stage descriptor requires non-empty id and name");
    }
    if (!stage) {
      return Error(humanoid::common::StatusCode::kInvalidArgument, "stage is null");
    }

    std::unique_lock<std::shared_mutex> lock{mutex_};
    if (stages_.contains(descriptor.stageId)) {
      return Error(humanoid::common::StatusCode::kFailedPrecondition,
                   "stage id is already registered");
    }
    for (const std::string& dependency : descriptor.dependencies) {
      if (dependency.empty()) {
        return Error(humanoid::common::StatusCode::kInvalidArgument,
                     "stage dependency id is empty");
      }
    }

    std::string stage_id = descriptor.stageId;
    stages_.emplace(std::move(stage_id), StageEntry{std::move(descriptor), std::move(stage)});
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status UnregisterStage(std::string_view stage_id) {
    if (stage_id.empty()) {
      return Error(humanoid::common::StatusCode::kInvalidArgument, "stage id is empty");
    }

    std::unique_lock<std::shared_mutex> lock{mutex_};
    const std::string key{stage_id};
    if (stages_.erase(key) == 0U) {
      return Error(humanoid::common::StatusCode::kUnavailable, "stage id is not registered");
    }
    for (auto& [registered_id, entry] : stages_) {
      static_cast<void>(registered_id);
      auto& dependencies = entry.descriptor.dependencies;
      dependencies.erase(std::remove(dependencies.begin(), dependencies.end(), key),
                         dependencies.end());
    }
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status ConfigureStage(std::string_view stage_id,
                                                        PerceptionStageConfig configuration) {
    std::unique_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = stages_.find(std::string{stage_id});
    if (iterator == stages_.end()) {
      return Error(humanoid::common::StatusCode::kUnavailable, "stage id is not registered");
    }
    iterator->second.descriptor.configuration = std::move(configuration);
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status SetStageEnabled(std::string_view stage_id, bool enabled) {
    std::unique_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = stages_.find(std::string{stage_id});
    if (iterator == stages_.end()) {
      return Error(humanoid::common::StatusCode::kUnavailable, "stage id is not registered");
    }
    iterator->second.descriptor.enabled = enabled;
    return humanoid::common::Status::ok();
  }

  void Clear() {
    std::unique_lock<std::shared_mutex> lock{mutex_};
    stages_.clear();
  }

  [[nodiscard]] std::vector<PerceptionStageDescriptor> DescribeGraph() const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    std::vector<PerceptionStageDescriptor> descriptors;
    descriptors.reserve(stages_.size());
    for (const auto& [stage_id, entry] : stages_) {
      static_cast<void>(stage_id);
      descriptors.push_back(entry.descriptor);
    }
    return descriptors;
  }

  [[nodiscard]] PerceptionPipelineResult Execute(SensorFrame frame) {
    std::vector<StageEntry> execution_order;
    humanoid::common::Status order_status = BuildExecutionOrder(execution_order);
    if (!order_status.isOk()) {
      PerceptionPipelineResult result;
      result.status = std::move(order_status);
      result.context.frame = std::move(frame);
      result.context.timestamp = Now();
      return result;
    }

    std::lock_guard<std::mutex> execution_lock{execution_mutex_};
    PerceptionPipelineResult result;
    result.context.frame = std::move(frame);
    result.context.timestamp = Now();

    for (const StageEntry& entry : execution_order) {
      if (!entry.descriptor.enabled) {
        continue;
      }

      try {
        humanoid::common::Status status = entry.stage->Process(result.context);
        if (!status.isOk()) {
          result.status = std::move(status);
          return result;
        }
        result.executedStages.push_back(entry.descriptor.stageId);
      } catch (...) {
        result.status = Error(humanoid::common::StatusCode::kInternalError,
                              "perception stage threw an exception");
        return result;
      }
    }

    result.status = humanoid::common::Status::ok();
    return result;
  }

  [[nodiscard]] std::size_t Size() const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    return stages_.size();
  }

private:
  struct StageEntry final {
    PerceptionStageDescriptor descriptor;
    std::shared_ptr<IPerceptionStage> stage;
  };

  [[nodiscard]] humanoid::common::Status
  BuildExecutionOrder(std::vector<StageEntry>& execution_order) const {
    std::map<std::string, StageEntry, std::less<>> snapshot;
    {
      std::shared_lock<std::shared_mutex> lock{mutex_};
      snapshot = stages_;
    }

    std::map<std::string, std::size_t, std::less<>> incoming_edges;
    std::map<std::string, std::vector<std::string>, std::less<>> dependents;
    for (const auto& [stage_id, entry] : snapshot) {
      incoming_edges.emplace(stage_id, 0U);
      for (const std::string& dependency : entry.descriptor.dependencies) {
        if (!snapshot.contains(dependency)) {
          return Error(humanoid::common::StatusCode::kFailedPrecondition,
                       "pipeline stage dependency is not registered");
        }
        ++incoming_edges[stage_id];
        dependents[dependency].push_back(stage_id);
      }
    }

    std::deque<std::string> ready;
    for (const auto& [stage_id, incoming_count] : incoming_edges) {
      if (incoming_count == 0U) {
        ready.push_back(stage_id);
      }
    }

    while (!ready.empty()) {
      const std::string stage_id = ready.front();
      ready.pop_front();
      execution_order.push_back(snapshot.at(stage_id));
      for (const std::string& dependent : dependents[stage_id]) {
        auto edge = incoming_edges.find(dependent);
        if (edge != incoming_edges.end() && edge->second > 0U) {
          --edge->second;
          if (edge->second == 0U) {
            ready.push_back(dependent);
          }
        }
      }
    }

    if (execution_order.size() != snapshot.size()) {
      return Error(humanoid::common::StatusCode::kFailedPrecondition,
                   "pipeline graph contains a cycle");
    }
    return humanoid::common::Status::ok();
  }

  mutable std::shared_mutex mutex_;
  mutable std::mutex execution_mutex_;
  std::map<std::string, StageEntry, std::less<>> stages_;
};

PerceptionPipeline::PerceptionPipeline() : impl_(std::make_unique<Impl>()) {}

PerceptionPipeline::~PerceptionPipeline() noexcept = default;

humanoid::common::Status
PerceptionPipeline::RegisterStage(PerceptionStageDescriptor descriptor,
                                  std::shared_ptr<IPerceptionStage> stage) {
  return impl_->RegisterStage(std::move(descriptor), std::move(stage));
}

humanoid::common::Status PerceptionPipeline::UnregisterStage(std::string_view stage_id) {
  return impl_->UnregisterStage(stage_id);
}

humanoid::common::Status PerceptionPipeline::ConfigureStage(std::string_view stage_id,
                                                            PerceptionStageConfig configuration) {
  return impl_->ConfigureStage(stage_id, std::move(configuration));
}

humanoid::common::Status PerceptionPipeline::SetStageEnabled(std::string_view stage_id,
                                                             bool enabled) {
  return impl_->SetStageEnabled(stage_id, enabled);
}

void PerceptionPipeline::Clear() { impl_->Clear(); }

std::vector<PerceptionStageDescriptor> PerceptionPipeline::DescribeGraph() const {
  return impl_->DescribeGraph();
}

PerceptionPipelineResult PerceptionPipeline::Execute(SensorFrame frame) {
  return impl_->Execute(std::move(frame));
}

std::size_t PerceptionPipeline::Size() const { return impl_->Size(); }

} // namespace humanoid::perception
