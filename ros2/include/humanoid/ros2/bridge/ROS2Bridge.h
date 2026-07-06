#pragma once

/**
 * @file ROS2Bridge.h
 * @brief Defines the optional ROS2 bridge facade for humanoid-core.
 *
 * The bridge header intentionally includes no ROS2 headers. Runtime-specific
 * ROS2 node implementations may depend on rclcpp inside the `ros2/` module,
 * but humanoid-core and applications that only use the framework remain
 * independent from ROS2.
 */

#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <humanoid/core/CommandDispatcher.h>
#include <humanoid/core/RobotAdapter.h>
#include <humanoid/core/RobotStateManager.hpp>
#include <humanoid/mission/MissionExecutor.h>
#include <humanoid/perception/DetectionResult.h>
#include <humanoid/ros2/actions/ActionCatalog.h>
#include <humanoid/ros2/messages/ROS2MessageTypes.h>
#include <humanoid/ros2/services/ServiceCatalog.h>
#include <humanoid/ros2/topics/TopicCatalog.h>
#include <humanoid/runtime/ExecutionContext.h>
#include <humanoid/services/TelemetryService.h>

namespace humanoid::ros2::bridge {

/**
 * @brief Optional dependencies injected into the ROS2 bridge.
 *
 * Null dependencies are accepted. The bridge reports unavailable operations as
 * rejected DTO results instead of dereferencing missing framework components.
 */
struct ROS2BridgeDependencies final {
  /** @brief Robot adapter used for connection and state queries. */
  std::shared_ptr<humanoid::core::RobotAdapter> robotAdapter;

  /** @brief Latest-state manager used for state publishing. */
  std::shared_ptr<humanoid::core::RobotStateManager> robotStateManager;

  /** @brief Telemetry service used for periodic state publication. */
  std::shared_ptr<humanoid::services::TelemetryService> telemetryService;

  /** @brief Command dispatcher used by command subscribers and actions. */
  std::shared_ptr<humanoid::core::CommandDispatcher> commandDispatcher;

  /** @brief Mission executor used by mission services and actions. */
  std::shared_ptr<humanoid::mission::MissionExecutor> missionExecutor;

  /** @brief Runtime context used for runtime status publication. */
  std::shared_ptr<humanoid::runtime::ExecutionContext> executionContext;
};

/**
 * @brief Static configuration for the ROS2 bridge.
 */
struct ROS2BridgeOptions final {
  /** @brief Logical bridge node name used by ROS2 runtime adapters. */
  std::string nodeName{"humanoid_core_bridge"};

  /** @brief Topic names published and subscribed by the bridge. */
  humanoid::ros2::topics::TopicCatalog topics{};

  /** @brief Service names exposed by the bridge. */
  humanoid::ros2::services::ServiceCatalog services{};

  /** @brief Action names exposed by the bridge. */
  humanoid::ros2::actions::ActionCatalog actions{};

  /** @brief True when telemetry subscription should start with the bridge. */
  bool subscribeTelemetry{true};
};

/**
 * @brief Immutable snapshot of bridge lifecycle state.
 */
struct ROS2BridgeStatus final {
  /** @brief True when Start() has completed and Stop() has not been called. */
  bool running{false};

  /** @brief True when a ROS2 runtime backend was detected at configure time. */
  bool ros2RuntimeAvailable{false};

  /** @brief Logical node name. */
  std::string nodeName;

  /** @brief Number of configured publisher topics. */
  std::size_t publisherCount{0U};

  /** @brief Number of configured subscriber topics. */
  std::size_t subscriberCount{0U};

  /** @brief Number of configured services. */
  std::size_t serviceCount{0U};

  /** @brief Number of configured actions. */
  std::size_t actionCount{0U};
};

/**
 * @brief Thread-safe optional bridge facade from ROS2 endpoints to humanoid-core.
 *
 * The class owns no ROS2 global state and performs no implicit initialization.
 * It keeps the bridge lifecycle explicit and DI-friendly. Runtime ROS2 node
 * implementations may wrap this class and translate generated ROS messages into
 * the DTOs declared in `humanoid::ros2::messages`.
 */
class ROS2Bridge final {
public:
  /**
   * @brief Constructs a bridge with injected framework dependencies.
   *
   * @param dependencies Framework dependencies used by bridge endpoints.
   * @param options Static bridge configuration.
   */
  explicit ROS2Bridge(ROS2BridgeDependencies dependencies, ROS2BridgeOptions options = {});

  /**
   * @brief Stops the bridge and releases telemetry subscriptions.
   */
  ~ROS2Bridge() noexcept;

  ROS2Bridge(const ROS2Bridge&) = delete;
  ROS2Bridge& operator=(const ROS2Bridge&) = delete;
  ROS2Bridge(ROS2Bridge&&) = delete;
  ROS2Bridge& operator=(ROS2Bridge&&) = delete;

  /**
   * @brief Starts bridge lifecycle and optional telemetry subscription.
   *
   * @return True when the bridge transitions to running.
   */
  [[nodiscard]] bool Start();

  /**
   * @brief Stops bridge lifecycle and removes telemetry subscription.
   */
  void Stop() noexcept;

  /**
   * @brief Reports whether the bridge is currently running.
   *
   * @return True when started and not stopped.
   */
  [[nodiscard]] bool IsRunning() const noexcept;

  /**
   * @brief Returns a lifecycle and endpoint-count snapshot.
   *
   * @return Bridge status snapshot.
   */
  [[nodiscard]] ROS2BridgeStatus Status() const;

  /**
   * @brief Returns the latest robot state DTO.
   *
   * @return Robot state DTO from RobotStateManager, RobotAdapter, or defaults.
   */
  [[nodiscard]] humanoid::ros2::messages::RobotStateMessage ReadRobotState() const;

  /**
   * @brief Sends a command DTO through the injected CommandDispatcher.
   *
   * @param command ROS2 bridge command DTO.
   * @return Command result DTO.
   */
  [[nodiscard]] humanoid::ros2::messages::CommandResultMessage
  SubmitCommand(const humanoid::ros2::messages::CommandMessage& command);

  /**
   * @brief Returns the latest telemetry state received from TelemetryService.
   *
   * @return Last telemetry state when available.
   */
  [[nodiscard]] std::optional<humanoid::ros2::messages::RobotStateMessage>
  LastTelemetryState() const;

private:
  ROS2BridgeDependencies dependencies_;
  ROS2BridgeOptions options_;

  mutable std::mutex mutex_;
  bool running_{false};
  std::optional<humanoid::services::TelemetryService::SubscriptionId> telemetry_subscription_;
  std::optional<humanoid::ros2::messages::RobotStateMessage> last_telemetry_state_;
};

} // namespace humanoid::ros2::bridge
