#pragma once

/**
 * @file SensorFrame.h
 * @brief Defines a generic sensor frame container.
 */

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/perception/SensorType.h>

namespace humanoid::perception {

/**
 * @brief Stable frame identifier type local to one sensor stream.
 */
using SensorFrameId = std::uint64_t;

/**
 * @brief Monotonic timestamp associated with frame capture or receipt.
 */
using SensorFrameTimestamp =
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

/**
 * @brief Supported metadata scalar types for sensor frames.
 */
using SensorFrameMetadataValue = std::variant<bool, std::int64_t, double, std::string>;

/**
 * @brief Ordered frame metadata map.
 *
 * Metadata is intended for calibration labels, encoding hints, stream names,
 * frame ids, and diagnostics. It must not store SDK objects, middleware
 * messages, file descriptors, raw pointers, or vendor handles.
 */
using SensorFrameMetadata = std::map<std::string, SensorFrameMetadataValue, std::less<>>;

/**
 * @brief Generic vendor-independent sensor frame.
 *
 * `SensorFrame` is a byte-oriented transport-neutral value type. Image, point
 * cloud, audio, IMU, radar, and custom data may be represented by metadata and
 * payload bytes without binding the core API to OpenCV, PCL, ROS2, DDS, or a
 * vendor SDK.
 */
struct SensorFrame final {
  /** @brief Producer-assigned frame identifier; zero means unassigned. */
  SensorFrameId id{0U};

  /** @brief Sensor category that produced this frame. */
  SensorType sensorType{SensorType::Custom};

  /** @brief Monotonic capture or receipt timestamp. */
  SensorFrameTimestamp timestamp{};

  /** @brief Frame width for image-like or range-image data; zero when unused. */
  std::uint32_t width{0U};

  /** @brief Frame height for image-like or range-image data; zero when unused. */
  std::uint32_t height{0U};

  /** @brief Number of channels, fields, or axes represented by each sample. */
  std::uint32_t channels{0U};

  /** @brief Bytes per sample element; zero when unknown or not applicable. */
  std::uint32_t bytesPerElement{0U};

  /** @brief Stable encoding label such as "rgb8", "depth32f", or "imu6". */
  std::string encoding;

  /** @brief Raw frame payload bytes owned by the frame value. */
  std::vector<std::byte> data;

  /** @brief Non-operational frame annotations and diagnostics. */
  SensorFrameMetadata metadata;

  /** @brief Constructs an empty frame. */
  SensorFrame() = default;

  /**
   * @brief Reports whether the frame carries an assigned identity.
   *
   * @return True when `id` is nonzero.
   */
  [[nodiscard]] constexpr bool hasIdentity() const noexcept { return id != 0U; }

  /**
   * @brief Reports whether the payload is empty.
   *
   * @return True when no payload bytes are stored.
   */
  [[nodiscard]] bool empty() const noexcept { return data.empty(); }

  /**
   * @brief Returns the payload byte count.
   *
   * @return Number of bytes stored in `data`.
   */
  [[nodiscard]] std::size_t sizeBytes() const noexcept { return data.size(); }

  /**
   * @brief Reports whether this frame is image-like.
   *
   * @return True when width and height are both nonzero.
   */
  [[nodiscard]] constexpr bool hasDimensions() const noexcept {
    return width != 0U && height != 0U;
  }
};

/**
 * @brief Result returned by `Sensor::ReadFrame()`.
 */
struct SensorFrameResult final {
  /** @brief Operation status for the read attempt. */
  humanoid::common::Status status{humanoid::common::Status::ok()};

  /** @brief Frame value populated when status is successful. */
  SensorFrame frame;

  /** @brief Constructs a successful empty result. */
  SensorFrameResult() = default;

  /**
   * @brief Constructs a frame result.
   *
   * @param result_status Operation status.
   * @param result_frame Frame value.
   */
  SensorFrameResult(humanoid::common::Status result_status, SensorFrame result_frame)
      : status(std::move(result_status)), frame(std::move(result_frame)) {}

  /**
   * @brief Reports whether the read succeeded.
   *
   * @return True when `status` is OK.
   */
  [[nodiscard]] bool succeeded() const noexcept { return status.isOk(); }
};

} // namespace humanoid::perception
