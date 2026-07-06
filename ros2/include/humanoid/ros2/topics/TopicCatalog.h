#pragma once

/**
 * @file TopicCatalog.h
 * @brief Defines ROS2 topic names used by the optional bridge.
 */

#include <array>
#include <string_view>

namespace humanoid::ros2::topics {

/**
 * @brief ROS2 publisher topic names exposed by the bridge.
 */
struct PublisherTopics final {
  std::string_view robotState{"/humanoid/robot_state"};       ///< Robot state.
  std::string_view telemetry{"/humanoid/telemetry"};          ///< Telemetry.
  std::string_view battery{"/humanoid/battery"};              ///< Battery state.
  std::string_view jointState{"/humanoid/joint_state"};       ///< Joint state.
  std::string_view pose{"/humanoid/pose"};                    ///< Pose.
  std::string_view velocity{"/humanoid/velocity"};            ///< Velocity.
  std::string_view diagnostics{"/humanoid/diagnostics"};      ///< Diagnostics.
  std::string_view plannerStatus{"/humanoid/planner/status"}; ///< Planner status.
  std::string_view missionStatus{"/humanoid/mission/status"}; ///< Mission status.
  std::string_view behaviorTreeStatus{"/humanoid/bt/status"}; ///< Behavior tree status.
  std::string_view perceptionResult{"/humanoid/perception"};  ///< Perception result.
  std::string_view detectionResult{"/humanoid/detections"};   ///< Detection result.
  std::string_view runtimeStatus{"/humanoid/runtime/status"}; ///< Runtime status.
};

/**
 * @brief ROS2 subscriber topic names consumed by the bridge.
 */
struct SubscriberTopics final {
  std::string_view command{"/humanoid/command"};                ///< Command request.
  std::string_view missionRequest{"/humanoid/mission/request"}; ///< Mission request.
  std::string_view plannerGoal{"/humanoid/planner/goal"};       ///< Planner goal.
  std::string_view emergencyStop{"/humanoid/emergency_stop"};   ///< Emergency stop.
  std::string_view robotMode{"/humanoid/robot_mode"};           ///< Robot mode.
};

/**
 * @brief Complete topic catalog for the ROS2 bridge.
 */
struct TopicCatalog final {
  PublisherTopics publishers{};   ///< Publisher topic names.
  SubscriberTopics subscribers{}; ///< Subscriber topic names.
};

/**
 * @brief Returns the number of publisher topics in the catalog.
 */
[[nodiscard]] constexpr std::size_t publisherTopicCount() noexcept { return 13U; }

/**
 * @brief Returns the number of subscriber topics in the catalog.
 */
[[nodiscard]] constexpr std::size_t subscriberTopicCount() noexcept { return 5U; }

} // namespace humanoid::ros2::topics
