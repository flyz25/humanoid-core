#include <humanoid/perception/SensorFusion.h>

#include <algorithm>
#include <chrono>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>

namespace humanoid::perception {
namespace {

[[nodiscard]] humanoid::common::Status Error(humanoid::common::StatusCode code,
                                             std::string message) {
  return humanoid::common::Status::error(code, std::move(message));
}

[[nodiscard]] bool ContainsSensorType(const std::vector<SensorFrame>& frames,
                                      SensorType sensor_type) {
  return std::any_of(frames.begin(), frames.end(), [sensor_type](const SensorFrame& frame) {
    return frame.sensorType == sensor_type;
  });
}

} // namespace

class FrameSynchronizer::Impl final {
public:
  explicit Impl(FrameSynchronizationPolicy policy) : policy_(std::move(policy)) {}

  [[nodiscard]] humanoid::common::Status SetPolicy(FrameSynchronizationPolicy policy) {
    if (!policy.isValid()) {
      return Error(humanoid::common::StatusCode::kInvalidArgument,
                   "frame synchronization maximum skew must be non-negative");
    }
    std::unique_lock<std::shared_mutex> lock{mutex_};
    policy_ = std::move(policy);
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] FrameSynchronizationPolicy Policy() const {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    return policy_;
  }

  [[nodiscard]] SynchronizedFrameSet Synchronize(std::vector<SensorFrame> frames) const {
    FrameSynchronizationPolicy policy = Policy();
    if (!policy.isValid()) {
      return SynchronizedFrameSet{Error(humanoid::common::StatusCode::kInvalidArgument,
                                        "frame synchronization maximum skew must be non-negative"),
                                  {},
                                  {},
                                  {}};
    }
    if (frames.empty()) {
      return SynchronizedFrameSet{
          Error(humanoid::common::StatusCode::kInvalidArgument, "no frames supplied"), {}, {}, {}};
    }

    for (const SensorType required_type : policy.requiredSensorTypes) {
      if (!ContainsSensorType(frames, required_type)) {
        return SynchronizedFrameSet{Error(humanoid::common::StatusCode::kFailedPrecondition,
                                          "required sensor type is missing"),
                                    {},
                                    {},
                                    {}};
      }
    }

    auto [earliest, latest] = std::minmax_element(
        frames.begin(), frames.end(), [](const SensorFrame& lhs, const SensorFrame& rhs) {
          return lhs.timestamp < rhs.timestamp;
        });
    if (earliest == frames.end() || latest == frames.end()) {
      return SynchronizedFrameSet{
          Error(humanoid::common::StatusCode::kInvalidArgument, "no frames supplied"), {}, {}, {}};
    }
    if (earliest->timestamp == SensorFrameTimestamp{} ||
        latest->timestamp == SensorFrameTimestamp{}) {
      return SynchronizedFrameSet{Error(humanoid::common::StatusCode::kInvalidArgument,
                                        "all frames require monotonic timestamps"),
                                  {},
                                  {},
                                  {}};
    }

    const SensorFrameTimestamp earliest_timestamp = earliest->timestamp;
    const SensorFrameTimestamp latest_timestamp = latest->timestamp;
    const auto spread = latest_timestamp - earliest_timestamp;
    if (spread > policy.maximumSkew) {
      return SynchronizedFrameSet{Error(humanoid::common::StatusCode::kFailedPrecondition,
                                        "frame timestamps exceed synchronization skew"),
                                  {},
                                  earliest_timestamp,
                                  latest_timestamp};
    }

    return SynchronizedFrameSet{humanoid::common::Status::ok(), std::move(frames),
                                earliest_timestamp, latest_timestamp};
  }

private:
  mutable std::shared_mutex mutex_;
  FrameSynchronizationPolicy policy_;
};

FrameSynchronizer::FrameSynchronizer(FrameSynchronizationPolicy policy)
    : impl_(std::make_unique<Impl>(std::move(policy))) {}

FrameSynchronizer::~FrameSynchronizer() noexcept = default;

humanoid::common::Status FrameSynchronizer::SetPolicy(FrameSynchronizationPolicy policy) {
  return impl_->SetPolicy(std::move(policy));
}

FrameSynchronizationPolicy FrameSynchronizer::Policy() const { return impl_->Policy(); }

SynchronizedFrameSet FrameSynchronizer::Synchronize(std::vector<SensorFrame> frames) const {
  return impl_->Synchronize(std::move(frames));
}

} // namespace humanoid::perception
