#pragma once

/**
 * @file CommandNode.h
 * @brief Defines a behavior tree leaf node backed by CommandDispatcher.
 */

#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include <humanoid/bt/BTNode.h>
#include <humanoid/bt/BTStatus.h>
#include <humanoid/core/Command.h>
#include <humanoid/core/CommandResult.h>

namespace humanoid::core {
class CommandDispatcher;
} // namespace humanoid::core

namespace humanoid::bt {

/**
 * @brief Leaf node that executes one generic command through CommandDispatcher.
 *
 * The node never calls a robot adapter directly and never includes SDK headers.
 * Command execution is submitted asynchronously, then polled on subsequent
 * ticks without busy waiting.
 */
class CommandNode final : public BTNode {
public:
  /**
   * @brief Constructs a command node.
   *
   * @param name Stable diagnostic node name.
   * @param dispatcher Shared command dispatcher dependency.
   * @param command Generic command value to execute.
   */
  explicit CommandNode(std::string name = "Command",
                       std::shared_ptr<core::CommandDispatcher> dispatcher = {},
                       core::Command command = {});

  /** @brief Cancels active queued work when destroyed. */
  ~CommandNode() noexcept override;

  CommandNode(const CommandNode&) = delete;
  CommandNode& operator=(const CommandNode&) = delete;
  CommandNode(CommandNode&&) = delete;
  CommandNode& operator=(CommandNode&&) = delete;

  /**
   * @brief Returns the stable command node name.
   *
   * @return Non-owning node name.
   */
  [[nodiscard]] std::string_view Name() const noexcept override;

  /**
   * @brief Validates dispatcher availability.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Idle` when dispatcher exists, otherwise `Failure`.
   */
  [[nodiscard]] BTStatus Initialize(BTContext& context) override;

  /**
   * @brief Starts or polls asynchronous command execution.
   *
   * @param context Runtime-backed behavior tree context.
   * @return `Running` while queued/running, `Success` when completed,
   * `Failure` when rejected/failed/timed out, or `Aborted` when cancelled.
   */
  [[nodiscard]] BTStatus Tick(BTContext& context) override;

  /**
   * @brief Cancels queued work and clears terminal state.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Reset(BTContext& context) override;

  /**
   * @brief Cancels queued work owned by this node.
   *
   * @param context Runtime-backed behavior tree context.
   */
  void Shutdown(BTContext& context) override;

private:
  void CancelActiveLocked() noexcept;

  mutable std::mutex mutex_;
  std::string name_;
  std::shared_ptr<core::CommandDispatcher> dispatcher_;
  core::Command command_;
  std::optional<std::future<core::CommandResult>> future_;
  std::optional<BTStatus> terminal_status_;
};

} // namespace humanoid::bt
