#include <humanoid/runtime/RuntimeScheduler.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

namespace humanoid::runtime {
namespace {

[[nodiscard]] RuntimeSchedulerTimestamp Now() {
  return std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
}

[[nodiscard]] bool IsKnownPriority(RuntimeJobPriority priority) noexcept {
  switch (priority) {
  case RuntimeJobPriority::Low:
  case RuntimeJobPriority::Normal:
  case RuntimeJobPriority::High:
  case RuntimeJobPriority::Critical:
    return true;
  }
  return false;
}

[[nodiscard]] int PriorityRank(RuntimeJobPriority priority) noexcept {
  switch (priority) {
  case RuntimeJobPriority::Low:
    return 0;
  case RuntimeJobPriority::Normal:
    return 1;
  case RuntimeJobPriority::High:
    return 2;
  case RuntimeJobPriority::Critical:
    return 3;
  }
  return -1;
}

[[nodiscard]] bool IsKnownExecutionMode(RuntimeExecutionMode mode) noexcept {
  switch (mode) {
  case RuntimeExecutionMode::Parallel:
  case RuntimeExecutionMode::Sequential:
    return true;
  }
  return false;
}

[[nodiscard]] bool IsSuccessfulTerminal(ExecutionState state) noexcept {
  return state == ExecutionState::Completed;
}

[[nodiscard]] RuntimeJobResult Result(ExecutionState state, std::string message) {
  RuntimeJobResult result;
  result.state = state;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] std::shared_future<RuntimeJobResult> ReadyFuture(RuntimeJobResult result) {
  std::promise<RuntimeJobResult> promise;
  std::shared_future<RuntimeJobResult> future = promise.get_future().share();
  promise.set_value(std::move(result));
  return future;
}

} // namespace

namespace detail {

class RuntimeJobControl final {
public:
  RuntimeJobControl(RuntimeJobId job_id, ExecutionScope scope, ExecutionMetadata metadata)
      : job_id_(job_id), execution_(job_id, scope, 0U, std::move(metadata)) {}

  [[nodiscard]] RuntimeJobId JobId() const noexcept { return job_id_; }

  [[nodiscard]] ExecutionContext& Execution() noexcept { return execution_; }

  [[nodiscard]] CancellationToken Cancellation() const noexcept { return cancellation_.Token(); }

  [[nodiscard]] bool IsCancellationRequested() const noexcept {
    return cancellation_.IsCancelled() || execution_.CancellationRequested();
  }

  [[nodiscard]] bool IsPaused() const noexcept {
    std::lock_guard<std::mutex> lock{mutex_};
    return paused_;
  }

  void SetPaused(bool paused) {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      paused_ = paused;
    }
    condition_.notify_all();
  }

  [[nodiscard]] bool WaitIfPaused() {
    std::unique_lock<std::mutex> lock{mutex_};
    condition_.wait(lock, [this]() { return !paused_ || IsCancellationRequested(); });
    return !IsCancellationRequested();
  }

  [[nodiscard]] bool RequestStop() noexcept {
    const bool cancelled = cancellation_.Cancel();
    const bool context_cancelled = execution_.RequestCancellation();
    {
      std::lock_guard<std::mutex> lock{mutex_};
      paused_ = false;
    }
    condition_.notify_all();
    return cancelled || context_cancelled;
  }

private:
  const RuntimeJobId job_id_;
  ExecutionContext execution_;
  CancellationSource cancellation_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  bool paused_{false};
};

} // namespace detail

RuntimeJobContext::RuntimeJobContext(std::shared_ptr<detail::RuntimeJobControl> control)
    : control_(std::move(control)) {}

RuntimeJobId RuntimeJobContext::JobId() const noexcept { return control_ ? control_->JobId() : 0U; }

ExecutionContext& RuntimeJobContext::Execution() const {
  if (!control_) {
    throw std::logic_error{"RuntimeJobContext is invalid"};
  }
  return control_->Execution();
}

std::shared_ptr<ExecutionContext> RuntimeJobContext::SharedExecution() const {
  if (!control_) {
    throw std::logic_error{"RuntimeJobContext is invalid"};
  }
  return std::shared_ptr<ExecutionContext>{control_, &control_->Execution()};
}

CancellationToken RuntimeJobContext::Cancellation() const noexcept {
  return control_ ? control_->Cancellation() : CancellationToken{};
}

bool RuntimeJobContext::IsCancellationRequested() const noexcept {
  return control_ != nullptr && control_->IsCancellationRequested();
}

bool RuntimeJobContext::IsPaused() const noexcept {
  return control_ != nullptr && control_->IsPaused();
}

bool RuntimeJobContext::WaitIfPaused() const {
  return control_ == nullptr || control_->WaitIfPaused();
}

class RuntimeScheduler::Impl final {
public:
  explicit Impl(RuntimeSchedulerOptions options) : options_(options) {
    if (options_.maximumQueueSize == 0U) {
      throw std::invalid_argument{"RuntimeScheduler maximum queue size must be greater than zero"};
    }
    if (options_.workerCount == 0U) {
      throw std::invalid_argument{"RuntimeScheduler worker count must be greater than zero"};
    }

    try {
      worker_ids_.resize(options_.workerCount);
      workers_.reserve(options_.workerCount);
      for (std::size_t worker_index = 0U; worker_index < options_.workerCount; ++worker_index) {
        workers_.emplace_back([this, worker_index](std::stop_token stop_token) {
          Run(std::move(stop_token), worker_index);
        });
      }
    } catch (...) {
      {
        std::lock_guard<std::mutex> lock{mutex_};
        accepting_ = false;
      }
      for (std::jthread& worker : workers_) {
        worker.request_stop();
      }
      condition_.notify_all();
      for (std::jthread& worker : workers_) {
        if (worker.joinable()) {
          worker.join();
        }
      }
      throw;
    }
  }

  ~Impl() noexcept { Shutdown(); }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  [[nodiscard]] RuntimeJobHandle Submit(RuntimeJob job) {
    if (const std::optional<RuntimeJobResult> validation = Validate(job)) {
      RecordRejected();
      return RuntimeJobHandle{job.id, ReadyFuture(*validation)};
    }

    const RuntimeJobId job_id = job.id;
    std::shared_ptr<ScheduledJob> scheduled;
    RuntimeJobHandle handle;
    bool accepted = false;
    RuntimeJobResult immediate_result;

    {
      std::lock_guard<std::mutex> lock{mutex_};
      if (!accepting_) {
        immediate_result = Result(ExecutionState::Failed, "RuntimeScheduler is shut down");
        ++statistics_.rejected;
      } else if (records_.contains(job_id)) {
        immediate_result = Result(ExecutionState::Failed, "Runtime job identifier already exists");
        ++statistics_.rejected;
      } else if (queue_.size() >= options_.maximumQueueSize) {
        immediate_result = Result(ExecutionState::Failed, "RuntimeScheduler queue is full");
        ++statistics_.rejected;
      } else {
        try {
          scheduled = std::make_shared<ScheduledJob>(std::move(job), next_sequence_++);
          handle = RuntimeJobHandle{scheduled->job.id, scheduled->promise.get_future().share()};
          records_.emplace(scheduled->job.id, MakeInitialSnapshot(*scheduled));
          outstanding_.emplace(scheduled->job.id, scheduled);
          queue_.push_back(scheduled);
          ++statistics_.accepted;
          statistics_.highWatermark = std::max(statistics_.highWatermark, queue_.size());
          accepted = true;
        } catch (...) {
          records_.erase(scheduled->job.id);
          outstanding_.erase(scheduled->job.id);
          ++statistics_.failed;
          throw;
        }
      }
    }

    if (accepted) {
      condition_.notify_all();
    } else {
      handle = RuntimeJobHandle{job_id, ReadyFuture(std::move(immediate_result))};
    }
    return handle;
  }

  bool Pause(RuntimeJobId job_id) {
    std::shared_ptr<ScheduledJob> job;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      job = FindOutstandingLocked(job_id);
      if (!job || IsTerminalLocked(job_id)) {
        return false;
      }

      job->control->SetPaused(true);
      RuntimeJobSnapshot& record = records_.at(job_id);
      record.state = ExecutionState::Paused;
      record.result = Result(ExecutionState::Paused, "Runtime job paused");
    }
    condition_.notify_all();
    return true;
  }

  bool Resume(RuntimeJobId job_id) {
    std::shared_ptr<ScheduledJob> job;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      job = FindOutstandingLocked(job_id);
      if (!job || !job->control->IsPaused() || IsTerminalLocked(job_id)) {
        return false;
      }

      job->control->SetPaused(false);
      RuntimeJobSnapshot& record = records_.at(job_id);
      record.state = job->state == ScheduledJobState::Running ? ExecutionState::Running
                                                              : ExecutionState::Created;
      record.result = Result(record.state, "Runtime job resumed");
    }
    condition_.notify_all();
    return true;
  }

  bool Stop(RuntimeJobId job_id) {
    std::shared_ptr<ScheduledJob> job;
    bool queued = false;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      job = FindOutstandingLocked(job_id);
      if (!job || IsTerminalLocked(job_id)) {
        return false;
      }

      if (job->state == ScheduledJobState::Queued) {
        const auto queued_iterator = std::find(queue_.begin(), queue_.end(), job);
        if (queued_iterator != queue_.end()) {
          queue_.erase(queued_iterator);
        }
        outstanding_.erase(job_id);
        queued = true;
        ++statistics_.cancelled;
        ++statistics_.stopped;
        CompleteRecordLocked(
            job_id, Result(ExecutionState::Cancelled, "Runtime job stopped before execution"));
      } else {
        if (!job->stopRequested) {
          ++statistics_.stopped;
          job->stopRequested = true;
        }
        RuntimeJobSnapshot& record = records_.at(job_id);
        record.state = ExecutionState::Cancelled;
        record.result = Result(ExecutionState::Cancelled, "Runtime job stop requested");
      }
    }

    static_cast<void>(job->control->RequestStop());
    if (queued) {
      SetPromiseSafely(job,
                       Result(ExecutionState::Cancelled, "Runtime job stopped before execution"));
    }
    condition_.notify_all();
    return true;
  }

  void Shutdown() noexcept {
    std::deque<std::shared_ptr<ScheduledJob>> queued_jobs;
    std::vector<std::shared_ptr<ScheduledJob>> running_jobs;
    {
      std::unique_lock<std::mutex> lock{mutex_};
      if (shutdown_complete_) {
        return;
      }
      if (IsWorkerThread()) {
        accepting_ = false;
        return;
      }
      if (shutdown_in_progress_) {
        condition_.wait(lock, [this]() { return shutdown_complete_; });
        return;
      }

      shutdown_in_progress_ = true;
      accepting_ = false;
      queued_jobs.swap(queue_);
      for (const std::shared_ptr<ScheduledJob>& job : queued_jobs) {
        outstanding_.erase(job->job.id);
        ++statistics_.cancelled;
        CompleteRecordLocked(
            job->job.id,
            Result(ExecutionState::Cancelled, "Runtime job cancelled during scheduler shutdown"));
      }
      for (const auto& [id, job] : outstanding_) {
        static_cast<void>(id);
        running_jobs.push_back(job);
        if (!job->stopRequested) {
          ++statistics_.stopped;
          job->stopRequested = true;
        }
        RuntimeJobSnapshot& record = records_.at(job->job.id);
        record.state = ExecutionState::Cancelled;
        record.result = Result(ExecutionState::Cancelled,
                               "Runtime job cancellation requested during scheduler shutdown");
      }
    }

    for (const std::shared_ptr<ScheduledJob>& job : queued_jobs) {
      static_cast<void>(job->control->RequestStop());
      SetPromiseSafely(job, Result(ExecutionState::Cancelled,
                                   "Runtime job cancelled during scheduler shutdown"));
    }
    for (const std::shared_ptr<ScheduledJob>& job : running_jobs) {
      static_cast<void>(job->control->RequestStop());
    }

    for (std::jthread& worker : workers_) {
      worker.request_stop();
    }
    condition_.notify_all();
    for (std::jthread& worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }

    {
      std::lock_guard<std::mutex> lock{mutex_};
      shutdown_complete_ = true;
      shutdown_in_progress_ = false;
    }
    condition_.notify_all();
  }

  [[nodiscard]] std::optional<RuntimeJobSnapshot> GetSnapshot(RuntimeJobId job_id) const {
    std::lock_guard<std::mutex> lock{mutex_};
    const auto record = records_.find(job_id);
    if (record == records_.end()) {
      return std::nullopt;
    }
    return record->second;
  }

  [[nodiscard]] std::vector<RuntimeJobSnapshot> GetSnapshots() const {
    std::lock_guard<std::mutex> lock{mutex_};
    std::vector<RuntimeJobSnapshot> snapshots;
    snapshots.reserve(records_.size());
    for (const auto& [id, snapshot] : records_) {
      static_cast<void>(id);
      snapshots.push_back(snapshot);
    }
    return snapshots;
  }

  [[nodiscard]] RuntimeSchedulerStatistics GetStatistics() const {
    std::lock_guard<std::mutex> lock{mutex_};
    RuntimeSchedulerStatistics statistics = statistics_;
    statistics.queued = queue_.size();
    statistics.running = running_count_;
    statistics.paused = CountPausedLocked();
    statistics.maximumQueueSize = options_.maximumQueueSize;
    statistics.workerCount = options_.workerCount;
    return statistics;
  }

  [[nodiscard]] std::size_t Size() const {
    std::lock_guard<std::mutex> lock{mutex_};
    return queue_.size();
  }

private:
  enum class ScheduledJobState : std::uint8_t { Queued, Running };

  struct ScheduledJob final {
    ScheduledJob(RuntimeJob runtime_job, RuntimeJobSequence sequence_value)
        : job(std::move(runtime_job)), sequence(sequence_value),
          control(std::make_shared<detail::RuntimeJobControl>(job.id, job.scope, job.metadata)) {}

    RuntimeJob job;
    RuntimeJobSequence sequence{0U};
    std::shared_ptr<detail::RuntimeJobControl> control;
    std::promise<RuntimeJobResult> promise;
    ScheduledJobState state{ScheduledJobState::Queued};
    bool stopRequested{false};
  };

  [[nodiscard]] std::optional<RuntimeJobResult> Validate(const RuntimeJob& job) const {
    if (job.id == 0U) {
      return Result(ExecutionState::Failed, "Runtime job identifier must be nonzero");
    }
    if (!IsKnownPriority(job.priority)) {
      return Result(ExecutionState::Failed, "Runtime job priority is invalid");
    }
    if (!IsKnownExecutionMode(job.executionMode)) {
      return Result(ExecutionState::Failed, "Runtime job execution mode is invalid");
    }
    if (!job.callback) {
      return Result(ExecutionState::Failed, "Runtime job callback is required");
    }
    return std::nullopt;
  }

  void RecordRejected() {
    std::lock_guard<std::mutex> lock{mutex_};
    ++statistics_.rejected;
  }

  [[nodiscard]] RuntimeJobSnapshot MakeInitialSnapshot(const ScheduledJob& job) const {
    RuntimeJobSnapshot snapshot;
    snapshot.jobId = job.job.id;
    snapshot.sequence = job.sequence;
    snapshot.priority = job.job.priority;
    snapshot.executionMode = job.job.executionMode;
    snapshot.state = ExecutionState::Created;
    snapshot.queuedAt = Now();
    snapshot.result = Result(ExecutionState::Created, "Runtime job queued");
    return snapshot;
  }

  [[nodiscard]] std::shared_ptr<ScheduledJob> FindOutstandingLocked(RuntimeJobId job_id) const {
    const auto iterator = outstanding_.find(job_id);
    if (iterator == outstanding_.end()) {
      return {};
    }
    return iterator->second;
  }

  [[nodiscard]] bool IsTerminalLocked(RuntimeJobId job_id) const {
    const auto record = records_.find(job_id);
    return record != records_.end() && isTerminal(record->second.state);
  }

  [[nodiscard]] bool IsWorkerThread() const noexcept {
    const std::thread::id current_thread = std::this_thread::get_id();
    return std::find(worker_ids_.begin(), worker_ids_.end(), current_thread) != worker_ids_.end();
  }

  [[nodiscard]] std::size_t CountPausedLocked() const {
    std::size_t paused = 0U;
    for (const auto& [id, job] : outstanding_) {
      static_cast<void>(id);
      if (job->control->IsPaused()) {
        ++paused;
      }
    }
    return paused;
  }

  [[nodiscard]] bool CanStartLocked(const ScheduledJob& job) const noexcept {
    if (job.control->IsPaused()) {
      return false;
    }
    if (sequential_running_) {
      return false;
    }
    if (job.job.executionMode == RuntimeExecutionMode::Sequential && running_count_ > 0U) {
      return false;
    }
    return true;
  }

  [[nodiscard]] std::deque<std::shared_ptr<ScheduledJob>>::iterator FindNextJobLocked() {
    auto best = queue_.end();
    for (auto iterator = queue_.begin(); iterator != queue_.end(); ++iterator) {
      if (!CanStartLocked(**iterator)) {
        continue;
      }
      if (best == queue_.end()) {
        best = iterator;
        continue;
      }
      const int candidate_priority = PriorityRank((*iterator)->job.priority);
      const int best_priority = PriorityRank((*best)->job.priority);
      if (candidate_priority > best_priority ||
          (candidate_priority == best_priority && (*iterator)->sequence < (*best)->sequence)) {
        best = iterator;
      }
    }
    return best;
  }

  [[nodiscard]] bool HasSchedulableJobLocked() { return FindNextJobLocked() != queue_.end(); }

  void CompleteRecordLocked(RuntimeJobId job_id, RuntimeJobResult result) {
    RuntimeJobSnapshot& record = records_.at(job_id);
    record.state = result.state;
    record.result = result;
    record.completedAt = Now();
    completed_order_.push_back(job_id);
    PruneCompletedHistoryLocked();
  }

  void PruneCompletedHistoryLocked() {
    while (options_.maximumCompletedHistorySize > 0U &&
           completed_order_.size() > options_.maximumCompletedHistorySize) {
      const RuntimeJobId oldest = completed_order_.front();
      completed_order_.pop_front();
      if (!outstanding_.contains(oldest)) {
        records_.erase(oldest);
      }
    }
  }

  void RecordTerminalStatisticsLocked(ExecutionState state) noexcept {
    switch (state) {
    case ExecutionState::Completed:
      ++statistics_.completed;
      break;
    case ExecutionState::Cancelled:
      ++statistics_.cancelled;
      break;
    case ExecutionState::Failed:
    case ExecutionState::Aborted:
      ++statistics_.failed;
      break;
    case ExecutionState::Created:
    case ExecutionState::Starting:
    case ExecutionState::Running:
    case ExecutionState::Paused:
      ++statistics_.failed;
      break;
    }
  }

  [[nodiscard]] RuntimeJobResult NormalizeResult(const std::shared_ptr<ScheduledJob>& job,
                                                 RuntimeJobResult result) const {
    if (job->control->IsCancellationRequested()) {
      return Result(ExecutionState::Cancelled, "Runtime job cancelled");
    }
    if (!isTerminal(result.state)) {
      return Result(ExecutionState::Completed,
                    result.message.empty() ? "Runtime job completed" : std::move(result.message));
    }
    if (!IsSuccessfulTerminal(result.state) && result.message.empty()) {
      result.message = "Runtime job did not complete successfully";
    }
    return result;
  }

  void SetPromiseSafely(const std::shared_ptr<ScheduledJob>& job,
                        RuntimeJobResult result) noexcept {
    try {
      job->promise.set_value(std::move(result));
    } catch (...) {
    }
  }

  void Run(std::stop_token stop_token, std::size_t worker_index) noexcept {
    {
      std::lock_guard<std::mutex> lock{mutex_};
      worker_ids_[worker_index] = std::this_thread::get_id();
    }

    while (true) {
      std::shared_ptr<ScheduledJob> job;
      {
        std::unique_lock<std::mutex> lock{mutex_};
        condition_.wait(lock, [this, &stop_token]() {
          return stop_token.stop_requested() || !accepting_ || HasSchedulableJobLocked();
        });

        const auto next_job = FindNextJobLocked();
        if (next_job == queue_.end()) {
          if (stop_token.stop_requested() || !accepting_) {
            worker_ids_[worker_index] = std::thread::id{};
            return;
          }
          continue;
        }

        job = *next_job;
        queue_.erase(next_job);
        job->state = ScheduledJobState::Running;
        ++running_count_;
        sequential_running_ = job->job.executionMode == RuntimeExecutionMode::Sequential;
        statistics_.maximumConcurrentRunning =
            std::max(statistics_.maximumConcurrentRunning, running_count_);
        ++statistics_.started;

        RuntimeJobSnapshot& record = records_.at(job->job.id);
        record.state = ExecutionState::Running;
        record.startedAt = Now();
        record.result = Result(ExecutionState::Running, "Runtime job running");
        job->control->Execution().SetStartTimestamp(record.startedAt.value());
        job->control->Execution().SetState(ExecutionState::Running);
      }

      RuntimeJobResult result;
      try {
        RuntimeJobContext context{job->control};
        result = NormalizeResult(job, job->job.callback(context));
      } catch (const std::exception& exception) {
        result = Result(ExecutionState::Failed, exception.what());
      } catch (...) {
        result = Result(ExecutionState::Failed, "Runtime job callback threw an unknown exception");
      }

      {
        std::lock_guard<std::mutex> lock{mutex_};
        if (running_count_ > 0U) {
          --running_count_;
        }
        if (job->job.executionMode == RuntimeExecutionMode::Sequential) {
          sequential_running_ = false;
        }
        outstanding_.erase(job->job.id);
        job->control->Execution().SetState(result.state);
        CompleteRecordLocked(job->job.id, result);
        RecordTerminalStatisticsLocked(result.state);
      }
      SetPromiseSafely(job, result);
      condition_.notify_all();
    }
  }

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  RuntimeSchedulerOptions options_;
  std::deque<std::shared_ptr<ScheduledJob>> queue_;
  std::unordered_map<RuntimeJobId, std::shared_ptr<ScheduledJob>> outstanding_;
  std::map<RuntimeJobId, RuntimeJobSnapshot> records_;
  std::deque<RuntimeJobId> completed_order_;
  std::vector<std::jthread> workers_;
  std::vector<std::thread::id> worker_ids_;
  RuntimeSchedulerStatistics statistics_;
  RuntimeJobSequence next_sequence_{1U};
  std::size_t running_count_{0U};
  bool sequential_running_{false};
  bool accepting_{true};
  bool shutdown_in_progress_{false};
  bool shutdown_complete_{false};
};

RuntimeScheduler::RuntimeScheduler(RuntimeSchedulerOptions options)
    : impl_(std::make_unique<Impl>(options)) {}

RuntimeScheduler::~RuntimeScheduler() noexcept = default;

RuntimeJobHandle RuntimeScheduler::Submit(RuntimeJob job) { return impl_->Submit(std::move(job)); }

bool RuntimeScheduler::Pause(RuntimeJobId job_id) { return impl_->Pause(job_id); }

bool RuntimeScheduler::Resume(RuntimeJobId job_id) { return impl_->Resume(job_id); }

bool RuntimeScheduler::Stop(RuntimeJobId job_id) { return impl_->Stop(job_id); }

void RuntimeScheduler::Shutdown() noexcept { impl_->Shutdown(); }

std::optional<RuntimeJobSnapshot> RuntimeScheduler::GetSnapshot(RuntimeJobId job_id) const {
  return impl_->GetSnapshot(job_id);
}

std::vector<RuntimeJobSnapshot> RuntimeScheduler::GetSnapshots() const {
  return impl_->GetSnapshots();
}

RuntimeSchedulerStatistics RuntimeScheduler::GetStatistics() const {
  return impl_->GetStatistics();
}

std::size_t RuntimeScheduler::Size() const { return impl_->Size(); }

} // namespace humanoid::runtime
