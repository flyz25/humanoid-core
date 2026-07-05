#pragma once

/**
 * @file DetectionResult.h
 * @brief Defines vendor-independent perception detection result values.
 */

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace humanoid::perception {

/**
 * @brief Generic detection category produced by perception components.
 */
enum class DetectionType : std::uint8_t {
  Object,               ///< Object detection.
  Pose,                 ///< Human or robot pose detection.
  Face,                 ///< Face detection.
  QrCode,               ///< QR code detection.
  Marker,               ///< Fiducial or visual marker detection.
  SemanticSegmentation, ///< Semantic segmentation region.
  Custom                ///< Application-defined detection.
};

/**
 * @brief Axis-aligned two-dimensional bounding box.
 */
struct BoundingBox2D final {
  /** @brief Left coordinate in source-frame units. */
  double x{0.0};

  /** @brief Top coordinate in source-frame units. */
  double y{0.0};

  /** @brief Box width in source-frame units. */
  double width{0.0};

  /** @brief Box height in source-frame units. */
  double height{0.0};

  /**
   * @brief Reports whether the box has positive area.
   *
   * @return True when width and height are positive.
   */
  [[nodiscard]] constexpr bool isValid() const noexcept { return width > 0.0 && height > 0.0; }
};

/**
 * @brief Three-dimensional position in a named coordinate frame.
 */
struct Position3D final {
  /** @brief X coordinate. */
  double x{0.0};

  /** @brief Y coordinate. */
  double y{0.0};

  /** @brief Z coordinate. */
  double z{0.0};

  /** @brief Coordinate frame identifier for this position. */
  std::string coordinateFrame;
};

/**
 * @brief Generic detection result produced by perception stages.
 */
struct DetectionResult final {
  /** @brief Stable detection identifier within the producing stage. */
  std::string id;

  /** @brief Detection category. */
  DetectionType type{DetectionType::Custom};

  /** @brief Confidence in range [0, 1] when known. */
  double confidence{0.0};

  /** @brief Human-readable class, marker, face, pose, or segment label. */
  std::string label;

  /** @brief Optional image-like bounding box. */
  BoundingBox2D boundingBox;

  /** @brief Optional 3D position when depth or fused coordinates are available. */
  std::optional<Position3D> position;

  /** @brief Monotonic timestamp associated with the detection. */
  std::chrono::steady_clock::time_point timestamp{};

  /** @brief Optional tracking identifier associated with this detection. */
  std::string trackingId;

  /**
   * @brief Reports whether the result has a usable identity.
   *
   * @return True when `id` is not empty.
   */
  [[nodiscard]] bool hasIdentity() const noexcept { return !id.empty(); }
};

/**
 * @brief Ordered collection of detection results.
 */
using DetectionResults = std::vector<DetectionResult>;

} // namespace humanoid::perception
