#pragma once

/**
 * @file ExecutionContext.h
 * @brief Defines the thread-safe shared execution runtime context.
 */

#include <chrono>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>

#include <humanoid/runtime/ExecutionContextId.h>
#include <humanoid/runtime/ExecutionMetadata.h>
#include <humanoid/runtime/ExecutionScope.h>
#include <humanoid/runtime/ExecutionState.h>

namespace humanoid::runtime {

/**
 * @brief Monotonic timestamp used by the execution runtime.
 */
using ExecutionTimestamp =
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

/**
 * @brief Value snapshot returned by `ExecutionContext`.
 */
struct ExecutionContextSnapshot final {
  /** @brief Immutable execution context identifier. */
  ExecutionContextId executionId{kInvalidExecutionContextId};

  /** @brief Associated mission identifier, or zero when not applicable. */
  ExecutionMissionId missionId{kInvalidExecutionMissionId};

  /** @brief Latest runtime lifecycle state. */
  ExecutionState state{ExecutionState::Created};

  /** @brief Monotonic start time, or empty before execution starts. */
  std::optional<ExecutionTimestamp> startTimestamp;

  /** @brief Current engine-owned step identifier, or empty when unassigned. */
  std::optional<ExecutionStepId> currentStep;

  /** @brief Framework subsystem that owns the execution. */
  ExecutionScope scope{ExecutionScope::Unknown};

  /** @brief True after cooperative cancellation has been requested. */
  bool cancellationRequested{false};

  /** @brief Copy of non-operational runtime metadata. */
  ExecutionMetadata metadata;
};

/**
 * @brief Thread-safe central state container for one runtime execution.
 *
 * The context stores only framework-owned identifiers, lifecycle state,
 * monotonic time, cooperative cancellation, and metadata. It has no dependency
 * on mission execution, behavior trees, planners, ROS2, robot adapters, or
 * vendor SDKs. Execution engines own lifecycle transition policy; this class
 * provides synchronized storage and does not impose an engine-specific state
 * machine.
 */
class ExecutionContext final {
public:
  /**
   * @brief Constructs an execution context.
   *
   * @param execution_id Stable execution identifier; zero is unassigned.
   * @param scope Framework subsystem that owns the execution.
   * @param mission_id Associated mission identifier; zero means not applicable.
   * @param metadata Initial non-operational runtime metadata.
   */
  explicit ExecutionContext(ExecutionContextId execution_id = kInvalidExecutionContextId,
                            ExecutionScope scope = ExecutionScope::Unknown,
                            ExecutionMissionId mission_id = kInvalidExecutionMissionId,
                            ExecutionMetadata metadata = {});

  /** @brief Destroys the context and its cancellation state. */
  ~ExecutionContext() = default;

  ExecutionContext(const ExecutionContext&) = delete;
  ExecutionContext& operator=(const ExecutionContext&) = delete;
  ExecutionContext(ExecutionContext&&) = delete;
  ExecutionContext& operator=(ExecutionContext&&) = delete;

  /**
   * @brief Returns the immutable execution identifier.
   *
   * @return Execution identifier.
   */
  [[nodiscard]] ExecutionContextId Id() const noexcept;

  /**
   * @brief Returns the immutable execution scope.
   *
   * @return Execution scope.
   */
  [[nodiscard]] ExecutionScope Scope() const noexcept;

  /**
   * @brief Returns the associated mission identifier.
   *
   * @return Mission identifier, or zero when not applicable.
   */
  [[nodiscard]] ExecutionMissionId MissionId() const;

  /**
   * @brief Updates the associated mission identifier.
   *
   * @param mission_id Mission identifier, or zero to clear the association.
   */
  void SetMissionId(ExecutionMissionId mission_id);

  /**
   * @brief Returns the latest execution state.
   *
   * @return Execution state snapshot.
   */
  [[nodiscard]] ExecutionState State() const;

  /**
   * @brief Stores an execution state selected by the owning engine.
   *
   * @param state New execution state.
   */
  void SetState(ExecutionState state);

  /**
   * @brief Returns the monotonic start timestamp.
   *
   * @return Start timestamp, or empty before startup begins.
   */
  [[nodiscard]] std::optional<ExecutionTimestamp> StartTimestamp() const;

  /**
   * @brief Stores the monotonic start timestamp.
   *
   * @param timestamp Timestamp selected by the owning execution engine.
   */
  void SetStartTimestamp(ExecutionTimestamp timestamp);

  /** @brief Clears the start timestamp. */
  void ClearStartTimestamp();

  /**
   * @brief Returns the current engine-owned step identifier.
   *
   * @return Step identifier, or empty when no step is active.
   */
  [[nodiscard]] std::optional<ExecutionStepId> CurrentStep() const;

  /**
   * @brief Stores the current engine-owned step identifier.
   *
   * @param step_id Current step identifier.
   */
  void SetCurrentStep(ExecutionStepId step_id);

  /** @brief Clears the current step identifier. */
  void ClearCurrentStep();

  /**
   * @brief Requests cooperative cancellation.
   *
   * Cancellation is monotonic and cannot be reset for an existing context.
   *
   * @return True only for the first effective cancellation request.
   */
  [[nodiscard]] bool RequestCancellation() noexcept;

  /**
   * @brief Returns a cooperative cancellation token.
   *
   * @return Token sharing this context's cancellation state.
   */
  [[nodiscard]] std::stop_token CancellationToken() const noexcept;

  /**
   * @brief Reports whether cancellation has been requested.
   *
   * @return True after the context receives a cancellation request.
   */
  [[nodiscard]] bool CancellationRequested() const noexcept;

  /**
   * @brief Returns a copy of all runtime metadata.
   *
   * @return Metadata snapshot.
   */
  [[nodiscard]] ExecutionMetadata Metadata() const;

  /**
   * @brief Replaces all runtime metadata.
   *
   * @param metadata New metadata collection.
   */
  void SetMetadata(ExecutionMetadata metadata);

  /**
   * @brief Adds or replaces one metadata value.
   *
   * @param key Metadata key.
   * @param value Metadata value.
   */
  void SetMetadataValue(std::string key, std::string value);

  /**
   * @brief Returns one metadata value.
   *
   * @param key Metadata key to find.
   * @return Copied value, or empty when the key does not exist.
   */
  [[nodiscard]] std::optional<std::string> MetadataValue(std::string_view key) const;

  /**
   * @brief Removes one metadata value.
   *
   * @param key Metadata key to remove.
   * @return True when a value was removed.
   */
  bool RemoveMetadata(std::string_view key);

  /**
   * @brief Returns a consistent copy of stored context state.
   *
   * Mutable stored fields are captured under one lock. Cancellation state is
   * sampled from the independently synchronized C++ stop state.
   *
   * @return Execution context value snapshot.
   */
  [[nodiscard]] ExecutionContextSnapshot Snapshot() const;

private:
  const ExecutionContextId execution_id_;
  const ExecutionScope scope_;
  mutable std::mutex mutex_;
  ExecutionMissionId mission_id_;
  ExecutionState state_{ExecutionState::Created};
  std::optional<ExecutionTimestamp> start_timestamp_;
  std::optional<ExecutionStepId> current_step_;
  std::stop_source cancellation_source_;
  ExecutionMetadata metadata_;
};

} // namespace humanoid::runtime
