/**
 * @file execution_context_unit_test.cpp
 * @brief Validates the thread-safe vendor-independent execution context.
 */

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <humanoid/runtime/ExecutionContext.h>

namespace {

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

void TestDefaultsAndEnums() {
  const humanoid::runtime::ExecutionContext context;
  const humanoid::runtime::ExecutionContextSnapshot snapshot = context.Snapshot();

  Check(snapshot.executionId == humanoid::runtime::kInvalidExecutionContextId,
        "Default execution ID is not invalid");
  Check(snapshot.missionId == humanoid::runtime::kInvalidExecutionMissionId,
        "Default mission ID is not invalid");
  Check(snapshot.state == humanoid::runtime::ExecutionState::Created,
        "Default execution state is not Created");
  Check(snapshot.scope == humanoid::runtime::ExecutionScope::Unknown,
        "Default execution scope is not Unknown");
  Check(!snapshot.startTimestamp.has_value(), "Default start timestamp is assigned");
  Check(!snapshot.currentStep.has_value(), "Default current step is assigned");
  Check(!snapshot.cancellationRequested, "Default context is cancelled");
  Check(snapshot.metadata.empty(), "Default metadata is not empty");
  Check(humanoid::runtime::toString(humanoid::runtime::ExecutionState::Aborted) == "Aborted",
        "Execution state name mismatch");
  Check(humanoid::runtime::toString(humanoid::runtime::ExecutionScope::BehaviorTree) ==
            "BehaviorTree",
        "Execution scope name mismatch");
  Check(humanoid::runtime::isTerminal(humanoid::runtime::ExecutionState::Completed),
        "Completed state is not terminal");
  Check(!humanoid::runtime::isTerminal(humanoid::runtime::ExecutionState::Paused),
        "Paused state is terminal");
}

void TestStateAndMetadataSnapshot() {
  humanoid::runtime::ExecutionMetadata metadata{{"trace_id", "trace-1"}};
  humanoid::runtime::ExecutionContext context{42U, humanoid::runtime::ExecutionScope::Mission, 7U,
                                              std::move(metadata)};
  const humanoid::runtime::ExecutionTimestamp start =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());

  context.SetState(humanoid::runtime::ExecutionState::Running);
  context.SetStartTimestamp(start);
  context.SetCurrentStep(3U);
  context.SetMetadataValue("owner", "mission-service");

  const humanoid::runtime::ExecutionContextSnapshot snapshot = context.Snapshot();
  Check(context.Id() == 42U, "Execution ID getter mismatch");
  Check(context.Scope() == humanoid::runtime::ExecutionScope::Mission, "Scope getter mismatch");
  Check(context.State() == humanoid::runtime::ExecutionState::Running, "State getter mismatch");
  Check(snapshot.executionId == 42U, "Execution ID mismatch");
  Check(snapshot.missionId == 7U, "Mission ID mismatch");
  Check(snapshot.scope == humanoid::runtime::ExecutionScope::Mission, "Scope mismatch");
  Check(snapshot.state == humanoid::runtime::ExecutionState::Running, "State mismatch");
  Check(snapshot.startTimestamp == start, "Start timestamp mismatch");
  Check(snapshot.currentStep == 3U, "Current step mismatch");
  Check(snapshot.metadata.at("trace_id") == "trace-1", "Initial metadata mismatch");
  Check(context.MetadataValue("owner") == "mission-service", "Metadata lookup mismatch");

  context.SetMissionId(8U);
  context.ClearStartTimestamp();
  context.ClearCurrentStep();
  Check(context.MissionId() == 8U, "Mission ID update failed");
  Check(!context.StartTimestamp().has_value(), "Start timestamp clear failed");
  Check(!context.CurrentStep().has_value(), "Current step clear failed");
  Check(context.RemoveMetadata("owner"), "Metadata removal failed");
  Check(!context.RemoveMetadata("owner"), "Missing metadata was removed");

  context.SetMetadata({{"replacement", "value"}});
  Check(context.Metadata().size() == 1U, "Metadata replacement did not replace all values");
  Check(context.MetadataValue("replacement") == "value", "Replacement metadata mismatch");
}

void TestCancellationToken() {
  humanoid::runtime::ExecutionContext context{1U, humanoid::runtime::ExecutionScope::Custom};
  const std::stop_token token = context.CancellationToken();

  Check(token.stop_possible(), "Cancellation token cannot receive cancellation");
  Check(!token.stop_requested(), "Cancellation token starts requested");
  Check(context.RequestCancellation(), "First cancellation request was ineffective");
  Check(!context.RequestCancellation(), "Second cancellation request was effective");
  Check(token.stop_requested(), "Cancellation did not propagate to copied token");
  Check(context.CancellationRequested(), "Context did not record cancellation");
}

void TestConcurrentAccess() {
  humanoid::runtime::ExecutionContext context{9U, humanoid::runtime::ExecutionScope::Mission};
  constexpr int kThreadCount = 8;
  constexpr int kIterations = 500;
  std::atomic<int> effective_cancellations{0};

  {
    std::vector<std::jthread> workers;
    workers.reserve(kThreadCount);
    for (int thread_index = 0; thread_index < kThreadCount; ++thread_index) {
      workers.emplace_back([&context, &effective_cancellations, thread_index]() {
        const std::string key = "worker-" + std::to_string(thread_index);
        for (int iteration = 0; iteration < kIterations; ++iteration) {
          context.SetCurrentStep(static_cast<humanoid::runtime::ExecutionStepId>(iteration + 1));
          context.SetState(iteration % 2 == 0 ? humanoid::runtime::ExecutionState::Running
                                              : humanoid::runtime::ExecutionState::Paused);
          context.SetMetadataValue(key, std::to_string(iteration));
          static_cast<void>(context.Snapshot());
        }
        if (context.RequestCancellation()) {
          ++effective_cancellations;
        }
      });
    }
  }

  const humanoid::runtime::ExecutionContextSnapshot snapshot = context.Snapshot();
  Check(snapshot.executionId == 9U, "Concurrent access changed execution ID");
  Check(snapshot.scope == humanoid::runtime::ExecutionScope::Mission,
        "Concurrent access changed execution scope");
  Check(snapshot.currentStep.has_value(), "Concurrent access lost current step");
  Check(snapshot.metadata.size() == static_cast<std::size_t>(kThreadCount),
        "Concurrent metadata updates were lost");
  Check(effective_cancellations.load() == 1, "Cancellation was not one-shot under concurrency");
}

} // namespace

int main() {
  try {
    TestDefaultsAndEnums();
    TestStateAndMetadataSnapshot();
    TestCancellationToken();
    TestConcurrentAccess();
  } catch (...) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
