#pragma once

/**
 * @file RobotAdapter.h
 * @brief Defines the vendor-independent robot adapter interface.
 */

#include <string>

#include <humanoid/common/Status.hpp>
#include <humanoid/common/Version.hpp>
#include <humanoid/core/Command.h>
#include <humanoid/core/CommandResult.h>
#include <humanoid/core/RobotState.hpp>
#include <humanoid/core/SafetyValidator.h>

namespace humanoid::core {

/**
 * @brief Vendor-independent descriptive information for a robot adapter.
 *
 * Empty strings indicate that a field is not available from the adapter or
 * robot. The structure intentionally contains no vendor SDK types and no
 * transport-specific handles.
 */
struct RobotInformation final {
  /**
   * @brief Robot vendor name, for example "ExampleVendor" or "Simulator".
   */
  std::string vendor;

  /**
   * @brief Robot model name, for example "G1", "H1", or "CustomHumanoid".
   */
  std::string model;

  /**
   * @brief Adapter implementation name.
   */
  std::string adapterName;

  /**
   * @brief Robot serial number when reported by the robot or configuration.
   */
  std::string serialNumber;

  /**
   * @brief Robot firmware version when reported by the robot or configuration.
   */
  std::string firmwareVersion;

  /**
   * @brief Robot hardware revision when reported by the robot or configuration.
   */
  std::string hardwareRevision;

  /**
   * @brief Framework API version targeted by this adapter interface.
   */
  humanoid::common::SemanticVersion adapterApiVersion{humanoid::common::apiVersion()};
};

/**
 * @brief Vendor-independent capability declaration for a robot adapter.
 *
 * Capabilities are declarative. They allow applications and future plugin hosts
 * to select compatible robots without inspecting vendor SDK details.
 */
struct RobotCapabilities final {
  /**
   * @brief True when the adapter can initialize and release its resources.
   */
  bool supportsLifecycle{false};

  /**
   * @brief True when the adapter can establish and close robot communication.
   */
  bool supportsConnectionManagement{false};

  /**
   * @brief True when the adapter can report a `RobotState` snapshot.
   */
  bool supportsStateFeedback{false};

  /**
   * @brief True when the adapter can report `RobotInformation`.
   */
  bool supportsRobotInformation{false};

  /**
   * @brief True when the adapter requires or supports periodic `Update()` calls.
   */
  bool supportsPeriodicUpdate{false};

  /**
   * @brief True when the adapter can expose battery or charging state.
   */
  bool supportsPowerState{false};

  /**
   * @brief True when the adapter can expose position or orientation estimates.
   */
  bool supportsPoseEstimation{false};

  /**
   * @brief True when the adapter can expose normalized fault or safety state.
   */
  bool supportsHealthState{false};

  /**
   * @brief True when the adapter can execute framework `Command` values.
   */
  bool supportsCommandExecution{false};
};

/**
 * @brief Generic robot adapter boundary for future robot implementations.
 *
 * `RobotAdapter` is a pure abstract interface. It defines lifecycle,
 * connection, state, information, capability, and update operations without
 * exposing vendor SDK headers, vendor enums, transport handles, or concrete
 * robot implementations.
 *
 * Applications and plugin hosts may depend on this interface. Concrete robot
 * adapters must remain outside core and must translate vendor-specific details
 * into the vendor-independent data types declared by humanoid-core.
 */
class RobotAdapter {
public:
  /**
   * @brief Destroys the adapter interface.
   */
  virtual ~RobotAdapter() = default;

  /**
   * @brief Initializes adapter resources without opening robot communication.
   *
   * Implementations should allocate local resources, validate static
   * configuration, and prepare internal state. Implementations must not expose
   * vendor exceptions through this boundary.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Initialize() = 0;

  /**
   * @brief Releases adapter resources and returns the adapter to an inactive state.
   *
   * Implementations should be safe to shut down after failed initialization,
   * failed connection, or a previous disconnect.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Shutdown() = 0;

  /**
   * @brief Establishes communication with the robot.
   *
   * The operation is vendor independent. Concrete adapters own the translation
   * to SDK-specific or transport-specific connection mechanisms.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Connect() = 0;

  /**
   * @brief Closes communication with the robot.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Disconnect() = 0;

  /**
   * @brief Reports whether communication is currently established.
   *
   * @return True when the adapter is connected to the robot.
   */
  [[nodiscard]] virtual bool IsConnected() const noexcept = 0;

  /**
   * @brief Returns the latest vendor-independent robot state snapshot.
   *
   * Implementations should return the latest known state without exposing SDK
   * types. When no state has been received yet, implementations should return a
   * default `RobotState` with conservative field values.
   *
   * @return Latest robot state snapshot.
   */
  [[nodiscard]] virtual RobotState GetRobotState() const = 0;

  /**
   * @brief Returns static or slowly changing robot information.
   *
   * @return Robot information normalized to framework data types.
   */
  [[nodiscard]] virtual RobotInformation GetRobotInformation() const = 0;

  /**
   * @brief Returns the adapter capability declaration.
   *
   * @return Vendor-independent adapter capabilities.
   */
  [[nodiscard]] virtual RobotCapabilities GetCapabilities() const = 0;

  /**
   * @brief Returns the generic command capability declaration.
   *
   * @return Vendor-independent command capability set.
   */
  [[nodiscard]] virtual CommandCapabilitySet GetCommandCapabilities() const = 0;

  /**
   * @brief Executes a validated framework command.
   *
   * The adapter must translate the command to vendor APIs internally and must
   * never expose SDK exceptions or vendor types through this boundary.
   *
   * @param command Vendor-independent command to execute.
   * @return Final command result.
   */
  [[nodiscard]] virtual CommandResult ExecuteCommand(const Command& command) = 0;

  /**
   * @brief Performs one non-blocking adapter update cycle.
   *
   * Implementations may use this to poll transport state, refresh cached robot
   * state, or process pending adapter events. The call should return promptly
   * and must not implement mission, planning, AI, or behavior logic.
   *
   * @return Operation status.
   */
  [[nodiscard]] virtual humanoid::common::Status Update() = 0;
};

} // namespace humanoid::core
