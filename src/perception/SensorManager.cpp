#include <humanoid/perception/SensorManager.h>

#include <chrono>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

namespace humanoid::perception {
namespace {

[[nodiscard]] SensorFrameTimestamp Now() noexcept {
  return std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
}

[[nodiscard]] humanoid::common::Status Error(humanoid::common::StatusCode code,
                                             std::string message) {
  return humanoid::common::Status::error(code, std::move(message));
}

[[nodiscard]] bool IsSuccess(const humanoid::common::Status& status) noexcept {
  return status.isOk();
}

} // namespace

class SensorManager::Impl final {
public:
  Impl() = default;

  ~Impl() noexcept {
    try {
      static_cast<void>(Shutdown());
    } catch (...) {
    }
  }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  [[nodiscard]] humanoid::common::Status RegisterSensor(std::unique_ptr<Sensor> sensor) {
    if (!sensor) {
      return Error(humanoid::common::StatusCode::kInvalidArgument, "sensor is null");
    }

    SensorCapabilities capabilities;
    try {
      capabilities = sensor->Capabilities();
    } catch (...) {
      return Error(humanoid::common::StatusCode::kInternalError,
                   "sensor capabilities query threw an exception");
    }

    if (!capabilities.isValid()) {
      return Error(humanoid::common::StatusCode::kInvalidArgument,
                   "sensor capabilities require non-empty sensor id and name");
    }

    auto entry = std::make_shared<Entry>(std::move(sensor), capabilities);
    std::unique_lock<std::shared_mutex> lock{mutex_};
    const auto [iterator, inserted] = sensors_.emplace(capabilities.sensorId, std::move(entry));
    static_cast<void>(iterator);
    if (!inserted) {
      return Error(humanoid::common::StatusCode::kFailedPrecondition,
                   "sensor id is already registered");
    }
    ++statistics_.registrations;
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status UnregisterSensor(std::string_view sensor_id) {
    if (sensor_id.empty()) {
      return Error(humanoid::common::StatusCode::kInvalidArgument, "sensor id is empty");
    }

    std::shared_ptr<Entry> entry;
    {
      std::unique_lock<std::shared_mutex> lock{mutex_};
      const auto iterator = sensors_.find(std::string{sensor_id});
      if (iterator == sensors_.end()) {
        return Error(humanoid::common::StatusCode::kUnavailable, "sensor id is not registered");
      }
      entry = iterator->second;
      sensors_.erase(iterator);
      ++statistics_.unregistrations;
    }

    MarkRemovedAndShutdown(*entry);
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] humanoid::common::Status InitializeSensor(std::string_view sensor_id) {
    return InvokeLifecycle(sensor_id, SensorLifecycleState::Initialized,
                           [](Sensor& sensor) { return sensor.Initialize(); });
  }

  [[nodiscard]] humanoid::common::Status StartSensor(std::string_view sensor_id) {
    return InvokeLifecycle(sensor_id, SensorLifecycleState::Running,
                           [](Sensor& sensor) { return sensor.Start(); });
  }

  [[nodiscard]] humanoid::common::Status StopSensor(std::string_view sensor_id) {
    return InvokeLifecycle(sensor_id, SensorLifecycleState::Stopped,
                           [](Sensor& sensor) { return sensor.Stop(); });
  }

  [[nodiscard]] humanoid::common::Status ShutdownSensor(std::string_view sensor_id) {
    return InvokeLifecycle(sensor_id, SensorLifecycleState::Shutdown,
                           [](Sensor& sensor) { return sensor.Shutdown(); });
  }

  [[nodiscard]] humanoid::common::Status Shutdown() {
    std::vector<std::shared_ptr<Entry>> entries;
    {
      std::unique_lock<std::shared_mutex> lock{mutex_};
      entries.reserve(sensors_.size());
      for (const auto& [sensor_id, entry] : sensors_) {
        static_cast<void>(sensor_id);
        entries.push_back(entry);
      }
      statistics_.unregistrations += entries.size();
      sensors_.clear();
    }

    for (const std::shared_ptr<Entry>& entry : entries) {
      MarkRemovedAndShutdown(*entry);
    }

    std::unique_lock<std::shared_mutex> lock{mutex_};
    listeners_.clear();
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] std::vector<SensorCapabilities> DiscoverSensors() const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    std::vector<SensorCapabilities> capabilities;
    capabilities.reserve(sensors_.size());
    for (const auto& [sensor_id, entry] : sensors_) {
      static_cast<void>(sensor_id);
      capabilities.push_back(entry->capabilities);
    }
    return capabilities;
  }

  [[nodiscard]] bool Contains(std::string_view sensor_id) const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    return sensors_.find(std::string{sensor_id}) != sensors_.end();
  }

  [[nodiscard]] std::size_t Size() const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    return sensors_.size();
  }

  [[nodiscard]] SensorManagerFrameResult ReadFrame(std::string_view sensor_id) {
    std::shared_ptr<Entry> entry = FindEntry(sensor_id);
    if (!entry) {
      return SensorManagerFrameResult{
          Error(humanoid::common::StatusCode::kUnavailable, "sensor id is not registered"),
          std::string{sensor_id}};
    }

    SensorManagerFrameResult result;
    {
      std::lock_guard<std::mutex> lock{entry->sensor_mutex};
      if (entry->removed) {
        return SensorManagerFrameResult{
            Error(humanoid::common::StatusCode::kUnavailable, "sensor is unregistered"),
            entry->capabilities.sensorId};
      }

      try {
        SensorFrameResult frame_result = entry->sensor->ReadFrame();
        if (!frame_result.succeeded()) {
          ++entry->state.frameErrors;
          entry->state.timestamp = Now();
          result = SensorManagerFrameResult{std::move(frame_result.status),
                                            entry->capabilities.sensorId};
        } else {
          SensorFrame frame = std::move(frame_result.frame);
          if (frame.timestamp == SensorFrameTimestamp{}) {
            frame.timestamp = Now();
          }
          frame.sensorType = entry->capabilities.type;

          ++entry->state.framesProduced;
          entry->state.timestamp = Now();
          entry->latest_frame = frame;
          result = SensorManagerFrameResult{std::move(frame_result.status),
                                            entry->capabilities.sensorId, std::move(frame)};
        }
      } catch (...) {
        ++entry->state.frameErrors;
        entry->state.timestamp = Now();
        result = SensorManagerFrameResult{Error(humanoid::common::StatusCode::kInternalError,
                                                "sensor frame read threw an exception"),
                                          entry->capabilities.sensorId};
      }
    }

    if (result.succeeded()) {
      RouteFrame(result.sensorId, result.frame);
    } else {
      RecordReadFailure();
    }
    return result;
  }

  [[nodiscard]] std::vector<SensorManagerFrameResult> ReadAllFrames() {
    std::vector<std::string> sensor_ids;
    {
      std::shared_lock<std::shared_mutex> lock{mutex_};
      sensor_ids.reserve(sensors_.size());
      for (const auto& [sensor_id, entry] : sensors_) {
        static_cast<void>(entry);
        sensor_ids.push_back(sensor_id);
      }
    }

    std::vector<SensorManagerFrameResult> results;
    results.reserve(sensor_ids.size());
    for (const std::string& sensor_id : sensor_ids) {
      results.push_back(ReadFrame(sensor_id));
    }
    return results;
  }

  [[nodiscard]] SensorManagerFrameResult LatestFrame(std::string_view sensor_id) const {
    std::shared_ptr<Entry> entry = FindEntry(sensor_id);
    if (!entry) {
      return SensorManagerFrameResult{
          Error(humanoid::common::StatusCode::kUnavailable, "sensor id is not registered"),
          std::string{sensor_id}};
    }

    std::lock_guard<std::mutex> lock{entry->sensor_mutex};
    if (!entry->latest_frame) {
      return SensorManagerFrameResult{
          Error(humanoid::common::StatusCode::kUnavailable, "sensor has no routed frame"),
          entry->capabilities.sensorId};
    }
    return SensorManagerFrameResult{humanoid::common::Status::ok(), entry->capabilities.sensorId,
                                    *entry->latest_frame};
  }

  [[nodiscard]] SensorHealthResult Health(std::string_view sensor_id) const {
    std::shared_ptr<Entry> entry = FindEntry(sensor_id);
    if (!entry) {
      return SensorHealthResult{
          Error(humanoid::common::StatusCode::kUnavailable, "sensor id is not registered"),
          std::string{sensor_id}};
    }

    std::lock_guard<std::mutex> lock{entry->sensor_mutex};
    try {
      SensorHealth health = entry->sensor->Health();
      if (health.timestamp == std::chrono::steady_clock::time_point{}) {
        health.timestamp = Now();
      }
      entry->state.health = health;
      entry->state.timestamp = Now();
      return SensorHealthResult{humanoid::common::Status::ok(), entry->capabilities.sensorId,
                                std::move(health)};
    } catch (...) {
      return SensorHealthResult{Error(humanoid::common::StatusCode::kInternalError,
                                      "sensor health query threw an exception"),
                                entry->capabilities.sensorId};
    }
  }

  [[nodiscard]] std::vector<SensorHealthResult> HealthReport() const {
    std::vector<std::string> sensor_ids;
    {
      std::shared_lock<std::shared_mutex> lock{mutex_};
      sensor_ids.reserve(sensors_.size());
      for (const auto& [sensor_id, entry] : sensors_) {
        static_cast<void>(entry);
        sensor_ids.push_back(sensor_id);
      }
    }

    std::vector<SensorHealthResult> results;
    results.reserve(sensor_ids.size());
    for (const std::string& sensor_id : sensor_ids) {
      results.push_back(Health(sensor_id));
    }
    return results;
  }

  [[nodiscard]] SensorStateResult State(std::string_view sensor_id) const {
    std::shared_ptr<Entry> entry = FindEntry(sensor_id);
    if (!entry) {
      return SensorStateResult{
          Error(humanoid::common::StatusCode::kUnavailable, "sensor id is not registered"),
          std::string{sensor_id}};
    }

    std::lock_guard<std::mutex> lock{entry->sensor_mutex};
    return SensorStateResult{humanoid::common::Status::ok(), entry->capabilities.sensorId,
                             entry->state};
  }

  [[nodiscard]] SensorFrameSubscriptionResult Subscribe(SensorFrameListener listener) {
    if (!listener) {
      return SensorFrameSubscriptionResult{
          Error(humanoid::common::StatusCode::kInvalidArgument, "frame listener is empty")};
    }

    std::unique_lock<std::shared_mutex> lock{mutex_};
    const SensorFrameSubscriptionId id = next_subscription_id_++;
    listeners_.emplace(id, std::move(listener));
    return SensorFrameSubscriptionResult{humanoid::common::Status::ok(), id};
  }

  [[nodiscard]] humanoid::common::Status Unsubscribe(SensorFrameSubscriptionId subscription_id) {
    if (subscription_id == 0U) {
      return Error(humanoid::common::StatusCode::kInvalidArgument, "subscription id is zero");
    }

    std::unique_lock<std::shared_mutex> lock{mutex_};
    if (listeners_.erase(subscription_id) == 0U) {
      return Error(humanoid::common::StatusCode::kUnavailable, "subscription id is not registered");
    }
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] SensorManagerStatistics GetStatistics() const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    SensorManagerStatistics statistics = statistics_;
    statistics.registeredSensors = sensors_.size();
    statistics.frameListeners = listeners_.size();
    return statistics;
  }

private:
  struct Entry final {
    Entry(std::unique_ptr<Sensor> entry_sensor, SensorCapabilities entry_capabilities)
        : sensor(std::move(entry_sensor)), capabilities(std::move(entry_capabilities)) {}

    std::unique_ptr<Sensor> sensor;
    SensorCapabilities capabilities;
    mutable std::mutex sensor_mutex;
    mutable SensorState state;
    std::optional<SensorFrame> latest_frame;
    bool removed{false};
  };

  template <typename Operation>
  [[nodiscard]] humanoid::common::Status InvokeLifecycle(std::string_view sensor_id,
                                                         SensorLifecycleState successful_state,
                                                         Operation operation) {
    std::shared_ptr<Entry> entry = FindEntry(sensor_id);
    if (!entry) {
      return Error(humanoid::common::StatusCode::kUnavailable, "sensor id is not registered");
    }

    std::lock_guard<std::mutex> lock{entry->sensor_mutex};
    if (entry->removed) {
      return Error(humanoid::common::StatusCode::kUnavailable, "sensor is unregistered");
    }

    try {
      humanoid::common::Status status = operation(*entry->sensor);
      entry->state.timestamp = Now();
      if (IsSuccess(status)) {
        entry->state.lifecycle = successful_state;
      }
      return status;
    } catch (...) {
      entry->state.lifecycle = SensorLifecycleState::Faulted;
      entry->state.timestamp = Now();
      return Error(humanoid::common::StatusCode::kInternalError,
                   "sensor lifecycle operation threw an exception");
    }
  }

  [[nodiscard]] std::shared_ptr<Entry> FindEntry(std::string_view sensor_id) const {
    if (sensor_id.empty()) {
      return {};
    }

    std::shared_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = sensors_.find(std::string{sensor_id});
    if (iterator == sensors_.end()) {
      return {};
    }
    return iterator->second;
  }

  void MarkRemovedAndShutdown(Entry& entry) noexcept {
    std::lock_guard<std::mutex> lock{entry.sensor_mutex};
    entry.removed = true;
    try {
      static_cast<void>(entry.sensor->Stop());
    } catch (...) {
    }
    try {
      static_cast<void>(entry.sensor->Shutdown());
    } catch (...) {
    }
    entry.state.lifecycle = SensorLifecycleState::Shutdown;
    entry.state.timestamp = Now();
  }

  void RouteFrame(const std::string& sensor_id, const SensorFrame& frame) {
    std::vector<SensorFrameListener> listeners;
    {
      std::unique_lock<std::shared_mutex> lock{mutex_};
      ++statistics_.routedFrames;
      listeners.reserve(listeners_.size());
      for (const auto& [subscription_id, listener] : listeners_) {
        static_cast<void>(subscription_id);
        listeners.push_back(listener);
      }
    }

    std::uint64_t callbacks = 0U;
    std::uint64_t failures = 0U;
    for (const SensorFrameListener& listener : listeners) {
      try {
        listener(sensor_id, frame);
        ++callbacks;
      } catch (...) {
        ++failures;
      }
    }

    if (callbacks != 0U || failures != 0U) {
      std::unique_lock<std::shared_mutex> lock{mutex_};
      statistics_.listenerCallbacks += callbacks;
      statistics_.listenerFailures += failures;
    }
  }

  void RecordReadFailure() {
    std::unique_lock<std::shared_mutex> lock{mutex_};
    ++statistics_.readFailures;
  }

  mutable std::shared_mutex mutex_;
  std::map<std::string, std::shared_ptr<Entry>, std::less<>> sensors_;
  std::map<SensorFrameSubscriptionId, SensorFrameListener> listeners_;
  SensorFrameSubscriptionId next_subscription_id_{1U};
  SensorManagerStatistics statistics_;
};

SensorManager::SensorManager() : impl_(std::make_unique<Impl>()) {}

SensorManager::~SensorManager() noexcept = default;

humanoid::common::Status SensorManager::RegisterSensor(std::unique_ptr<Sensor> sensor) {
  return impl_->RegisterSensor(std::move(sensor));
}

humanoid::common::Status SensorManager::UnregisterSensor(std::string_view sensor_id) {
  return impl_->UnregisterSensor(sensor_id);
}

humanoid::common::Status SensorManager::InitializeSensor(std::string_view sensor_id) {
  return impl_->InitializeSensor(sensor_id);
}

humanoid::common::Status SensorManager::StartSensor(std::string_view sensor_id) {
  return impl_->StartSensor(sensor_id);
}

humanoid::common::Status SensorManager::StopSensor(std::string_view sensor_id) {
  return impl_->StopSensor(sensor_id);
}

humanoid::common::Status SensorManager::ShutdownSensor(std::string_view sensor_id) {
  return impl_->ShutdownSensor(sensor_id);
}

humanoid::common::Status SensorManager::Shutdown() { return impl_->Shutdown(); }

std::vector<SensorCapabilities> SensorManager::DiscoverSensors() const {
  return impl_->DiscoverSensors();
}

bool SensorManager::Contains(std::string_view sensor_id) const {
  return impl_->Contains(sensor_id);
}

std::size_t SensorManager::Size() const { return impl_->Size(); }

SensorManagerFrameResult SensorManager::ReadFrame(std::string_view sensor_id) {
  return impl_->ReadFrame(sensor_id);
}

std::vector<SensorManagerFrameResult> SensorManager::ReadAllFrames() {
  return impl_->ReadAllFrames();
}

SensorManagerFrameResult SensorManager::LatestFrame(std::string_view sensor_id) const {
  return impl_->LatestFrame(sensor_id);
}

SensorHealthResult SensorManager::Health(std::string_view sensor_id) const {
  return impl_->Health(sensor_id);
}

std::vector<SensorHealthResult> SensorManager::HealthReport() const {
  return impl_->HealthReport();
}

SensorStateResult SensorManager::State(std::string_view sensor_id) const {
  return impl_->State(sensor_id);
}

SensorFrameSubscriptionResult SensorManager::Subscribe(SensorFrameListener listener) {
  return impl_->Subscribe(std::move(listener));
}

humanoid::common::Status SensorManager::Unsubscribe(SensorFrameSubscriptionId subscription_id) {
  return impl_->Unsubscribe(subscription_id);
}

SensorManagerStatistics SensorManager::GetStatistics() const { return impl_->GetStatistics(); }

} // namespace humanoid::perception
