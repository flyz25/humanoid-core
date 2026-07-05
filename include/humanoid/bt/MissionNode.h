#pragma once

/**
 * @file MissionNode.h
 * @brief Defines a behavior tree leaf node backed by MissionExecutor.
 */

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BTStatus.h>
#include <humanoid/mission/Mission.h>

namespace humanoid::mission {
class MissionExecutor;
} // namespace humanoid::mission

namespace humanoid::bt {

/**
 * @brief Leaf node that executes one mission through MissionExecutor.
 *
 * `MissionNode` starts mission execution through the mission framework and
 * polls executor status on subsequent ticks. It contains no mission parsing,
 * command dispatch bypass, robot adapter logic, or SDK dependency.
 */
class MissionNode final : public BTNode {
public:
  /**
   * @brief Constructs a mission node.
   *
   * @param name Stable diagnostic node name.
   * @param executor Shared mission executor dependency.
   * @param mission Mission value to execute.
   */
  explicit MissionNode(std::string name = "Mission",
                       std::shared_ptr<mission::MissionExecutor> executor = {},
                       mission::Mission mission = {});

  /** @brief Stops active node-owned mission work when destroyed. */
  ~MissionNode() noexcept override;

  MissionNode(const MissionNode&) = delete;
  MissionNode& operator=(const MissionNode&) = delete;
  MissionNode(MissionNode&&) = delete;
  MissionNode& operator=(MissionNode&&) = delete;

  /**
   * @brief Returns the stable mission node name.
   *
   * @return Non-owning node name.
   */
  [[nodiscard]] std::string_view Name() const noexcept override;

  /**
   * @brief Validates executor availability.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Idle` when executor exists, otherwise `Failure`.
   */
  [[nodiscard]] BTStatus Initialize(BTContext& context) override;

  /**
   * @brief Starts or polls mission execution.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Running` while mission is active, `Success` when completed,
   * `Failure` when failed, or `Aborted` when cancelled.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

  /**
   * @brief Stops active mission work and clears terminal state.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Reset(BTContext& context) override;

  /**
   * @brief Stops active mission work owned by this node.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Shutdown(BTContext& context) override;

private:
  void StopActiveLocked() noexcept;

  mutable std::mutex mutex_;
  std::string name_;
  std::shared_ptr<mission::MissionExecutor> executor_;
  mission::Mission mission_;
  bool started_{false};
  std::optional<BTStatus> terminal_status_;
};

} // namespace humanoid::bt
