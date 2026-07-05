#pragma once

/**
 * @file SensorFusion.h
 * @brief Defines generic sensor fusion interfaces and synchronization values.
 */

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/SensorFrame.h>
#include <humanoid/perception/SensorType.h>

namespace humanoid::perception {

/**
 * @brief Named coordinate frame used by perception components.
 */
struct CoordinateFrame final {
  /** @brief Stable coordinate frame identifier. */
  std::string frameId;

  /** @brief Optional parent coordinate frame identifier. */
  std::string parentFrameId;

  /** @brief Monotonic timestamp for this coordinate frame declaration. */
  std::chrono::steady_clock::time_point timestamp{};

  /**
   * @brief Reports whether the coordinate frame has an identity.
   *
   * @return True when frameId is not empty.
   */
  [[nodiscard]] bool isValid() const noexcept { return !frameId.empty(); }
};

/**
 * @brief Generic rigid transform metadata between coordinate frames.
 */
struct CoordinateTransform final {
  /** @brief Source coordinate frame. */
  CoordinateFrame source;

  /** @brief Target coordinate frame. */
  CoordinateFrame target;

  /** @brief Translation X component. */
  double translationX{0.0};

  /** @brief Translation Y component. */
  double translationY{0.0};

  /** @brief Translation Z component. */
  double translationZ{0.0};

  /** @brief Roll rotation in radians. */
  double roll{0.0};

  /** @brief Pitch rotation in radians. */
  double pitch{0.0};

  /** @brief Yaw rotation in radians. */
  double yaw{0.0};
};

/**
 * @brief Frame synchronization policy.
 */
struct FrameSynchronizationPolicy final {
  /** @brief Maximum timestamp spread accepted within one synchronized set. */
  std::chrono::nanoseconds maximumSkew{std::chrono::milliseconds{10}};

  /** @brief Required sensor types for a synchronized set. */
  std::vector<SensorType> requiredSensorTypes;

  /**
   * @brief Reports whether the policy is usable.
   *
   * @return True when maximumSkew is non-negative.
   */
  [[nodiscard]] constexpr bool isValid() const noexcept {
    return maximumSkew >= std::chrono::nanoseconds::zero();
  }
};

/**
 * @brief Synchronized collection of frames.
 */
struct SynchronizedFrameSet final {
  /** @brief Operation status for synchronization. */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /** @brief Frames accepted into the synchronized set. */
  std::vector<SensorFrame> frames;

  /** @brief Earliest timestamp in the accepted set. */
  SensorFrameTimestamp earliestTimestamp{};

  /** @brief Latest timestamp in the accepted set. */
  SensorFrameTimestamp latestTimestamp{};

  /**
   * @brief Reports whether synchronization succeeded.
   *
   * @return True when status is OK.
   */
  [[nodiscard]] bool succeeded() const noexcept { return status.isOk(); }
};

/**
 * @brief Fusion request passed to fusion implementations.
 */
struct FusionRequest final {
  /** @brief Synchronized input frames. */
  SynchronizedFrameSet synchronizedFrames;

  /** @brief Target coordinate frame for the fused output. */
  CoordinateFrame targetFrame;

  /** @brief Known coordinate transforms. */
  std::vector<CoordinateTransform> transforms;
};

/**
 * @brief Fusion output returned by fusion implementations.
 */
struct FusionResult final {
  /** @brief Operation status. */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /** @brief Fused frame or aggregate output. */
  SensorFrame outputFrame;

  /** @brief Coordinate frame associated with the output. */
  CoordinateFrame outputFrameCoordinates;

  /**
   * @brief Reports whether fusion succeeded.
   *
   * @return True when status is OK.
   */
  [[nodiscard]] bool succeeded() const noexcept { return status.isOk(); }
};

/**
 * @brief Pure abstract sensor fusion interface.
 */
class ISensorFusion {
public:
  /** @brief Destroys the fusion interface. */
  virtual ~ISensorFusion() = default;

  /**
   * @brief Fuses synchronized frames into one output.
   *
   * @param request Fusion request.
   * @return Fusion result.
   */
  [[nodiscard]] virtual FusionResult Fuse(const FusionRequest& request) = 0;
};

/**
 * @brief Thread-safe helper that aligns frames by timestamp.
 *
 * `FrameSynchronizer` performs only timestamp grouping. It does not implement
 * SLAM, localization, ROS2 TF, calibration estimation, tracking, or map
 * construction.
 */
class FrameSynchronizer final {
public:
  /** @brief Constructs a synchronizer with the supplied policy. */
  explicit FrameSynchronizer(FrameSynchronizationPolicy policy);

  /** @brief Destroys synchronization state. */
  ~FrameSynchronizer() noexcept;

  FrameSynchronizer(const FrameSynchronizer&) = delete;
  FrameSynchronizer& operator=(const FrameSynchronizer&) = delete;
  FrameSynchronizer(FrameSynchronizer&&) = delete;
  FrameSynchronizer& operator=(FrameSynchronizer&&) = delete;

  /**
   * @brief Updates the synchronization policy.
   *
   * @param policy New policy.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status SetPolicy(FrameSynchronizationPolicy policy);

  /**
   * @brief Returns the current synchronization policy.
   *
   * @return Policy snapshot.
   */
  [[nodiscard]] FrameSynchronizationPolicy Policy() const;

  /**
   * @brief Attempts to align supplied frames into a synchronized set.
   *
   * @param frames Candidate frames.
   * @return Synchronized set result.
   */
  [[nodiscard]] SynchronizedFrameSet Synchronize(std::vector<SensorFrame> frames) const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::perception
