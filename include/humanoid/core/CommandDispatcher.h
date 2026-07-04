#pragma once

/**
 * @file CommandDispatcher.h
 * @brief Defines the vendor-independent command dispatch service.
 */

#include <future>
#include <memory>

#include <humanoid/core/Command.h>
#include <humanoid/core/CommandResult.h>
#include <humanoid/core/SafetyValidator.h>

namespace humanoid::adapters {
class IRobotAdapter;
} // namespace humanoid::adapters

namespace humanoid::core {

class RobotStateManager;

/**
 * @brief Validates and forwards generic commands to an injected robot adapter.
 *
 * CommandDispatcher owns no vendor SDK objects and contains no robot business
 * logic. Supported generic command types are translated to the existing
 * `IRobotAdapter` interface. The dispatcher applies `SafetyValidator` before
 * adapter forwarding so disconnected, faulted, unsupported, or unsafe-state
 * commands are rejected. Adapter calls are serialized so synchronous and
 * asynchronous callers cannot invoke a non-thread-safe adapter concurrently.
 *
 * Asynchronous commands are ordered by priority and FIFO order within the same
 * priority. Cancellation is guaranteed only while a command remains queued;
 * the current adapter interface does not provide interruption of an operation
 * that has already started.
 *
 * All public operations are thread-safe. `Shutdown()` is idempotent, rejects
 * new work, cancels queued work, and waits for in-flight adapter calls.
 */
class CommandDispatcher final {
public:
  /**
   * @brief Constructs a dispatcher with an injected robot adapter.
   *
   * A null adapter is accepted so composition failures can be represented as
   * rejected command results instead of dereferencing an invalid dependency. The
   * default safety context uses the legacy adapter state query for connection
   * checks and the currently dispatchable legacy adapter capabilities.
   *
   * @param adapter Shared ownership of the adapter used for command forwarding.
   */
  explicit CommandDispatcher(std::shared_ptr<adapters::IRobotAdapter> adapter);

  /**
   * @brief Constructs a dispatcher with injected robot state and safety policy.
   *
   * When `state_manager` is non-null, each command is validated against the
   * latest `RobotStateManager` snapshot before adapter execution. When it is
   * null, the dispatcher falls back to the legacy adapter state query.
   *
   * @param adapter Shared ownership of the adapter used for command forwarding.
   * @param state_manager Optional latest-state source for safety validation.
   * @param capabilities Generic command capabilities for the active robot.
   * @param safety_validator Policy object used for safety validation.
   */
  CommandDispatcher(std::shared_ptr<adapters::IRobotAdapter> adapter,
                    std::shared_ptr<const RobotStateManager> state_manager,
                    CommandCapabilitySet capabilities,
                    SafetyValidator safety_validator = SafetyValidator{});

  /**
   * @brief Shuts down dispatch and releases internal worker resources.
   */
  ~CommandDispatcher() noexcept;

  CommandDispatcher(const CommandDispatcher&) = delete;
  CommandDispatcher& operator=(const CommandDispatcher&) = delete;
  CommandDispatcher(CommandDispatcher&&) = delete;
  CommandDispatcher& operator=(CommandDispatcher&&) = delete;

  /**
   * @brief Validates and executes a command on the calling thread.
   *
   * @param command Command to validate and forward.
   * @return Final command result.
   */
  [[nodiscard]] CommandResult Execute(const Command& command);

  /**
   * @brief Queues a command for asynchronous execution.
   *
   * Invalid, duplicate, unsupported, or post-shutdown commands produce a ready
   * future containing the corresponding terminal result.
   *
   * @param command Command value to queue.
   * @return Future containing the final command result.
   */
  [[nodiscard]] std::future<CommandResult> ExecuteAsync(Command command);

  /**
   * @brief Cancels a command that has not started adapter execution.
   *
   * @param command_id Identifier of the queued command to cancel.
   * @return `Cancelled` when queued work was cancelled, otherwise `Rejected`.
   */
  [[nodiscard]] CommandResult Cancel(CommandId command_id);

  /**
   * @brief Stops accepting commands and drains active adapter calls.
   *
   * Queued asynchronous commands complete with `CommandStatus::Cancelled`.
   * Commands already executing are allowed to finish because the adapter
   * interface has no interruption contract. This operation does not invoke the
   * robot adapter lifecycle `Shutdown()` function.
   *
   * @return Completed result when dispatcher shutdown is complete.
   */
  [[nodiscard]] CommandResult Shutdown();

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace humanoid::core
