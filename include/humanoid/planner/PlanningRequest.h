#pragma once

/**
 * @file PlanningRequest.h
 * @brief Defines planner input assembled from generic framework state.
 */

#include <humanoid/core/RobotAdapter.h>
#include <humanoid/planner/Goal.h>
#include <humanoid/runtime/ExecutionContext.h>

namespace humanoid::planner {

/**
 * @brief Runtime context snapshot supplied to a future planning boundary.
 *
 * A snapshot keeps the request copyable and avoids giving planner model code
 * ownership over a live runtime context.
 */
using RuntimeContext = humanoid::runtime::ExecutionContextSnapshot;

/**
 * @brief Vendor-independent request for converting intent into execution data.
 *
 * The request combines user intent, runtime state, and declared robot
 * capabilities. It contains no planner implementation, robot adapter instance,
 * SDK object, LLM client, network client, or parser dependency.
 */
struct PlanningRequest final {
  /**
   * @brief User intent to plan.
   */
  Goal goal;

  /**
   * @brief Copy of current runtime execution context.
   */
  RuntimeContext runtimeContext;

  /**
   * @brief Generic robot capability declaration available to the planner.
   */
  humanoid::core::RobotCapabilities robotCapabilities;

  /**
   * @brief Constructs an empty planning request.
   */
  PlanningRequest() = default;

  /**
   * @brief Reports whether the request contains a usable goal.
   *
   * @return True when the embedded goal passes minimum validation.
   */
  [[nodiscard]] bool isValid() const noexcept { return goal.isValid(); }
};

} // namespace humanoid::planner
