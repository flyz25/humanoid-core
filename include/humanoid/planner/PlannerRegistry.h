#pragma once

/**
 * @file PlannerRegistry.h
 * @brief Defines the thread-safe planner capability registry.
 */

#include <cstddef>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/planner/IPlanner.h>

namespace humanoid::planner {

/**
 * @brief Host-visible record for a registered planner.
 */
struct PlannerRecord final {
  /**
   * @brief Registered planner capabilities.
   */
  PlannerCapabilities capabilities{};
};

/**
 * @brief Thread-safe registry for planner capability metadata.
 *
 * `PlannerRegistry` owns no planner implementation and performs no dynamic
 * loading. It stores discovery metadata supplied by planner infrastructure or
 * application composition code.
 */
class PlannerRegistry final {
public:
  /**
   * @brief Constructs an empty planner registry.
   */
  PlannerRegistry() = default;

  /**
   * @brief Destroys the planner registry.
   */
  ~PlannerRegistry() = default;

  PlannerRegistry(const PlannerRegistry&) = delete;
  PlannerRegistry& operator=(const PlannerRegistry&) = delete;
  PlannerRegistry(PlannerRegistry&&) = delete;
  PlannerRegistry& operator=(PlannerRegistry&&) = delete;

  /**
   * @brief Registers planner capabilities.
   *
   * @param capabilities Planner capability declaration.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status RegisterPlanner(PlannerCapabilities capabilities);

  /**
   * @brief Removes a planner registration.
   *
   * @param planner_id Stable planner identifier.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status UnregisterPlanner(std::string_view planner_id);

  /**
   * @brief Reports whether a planner is registered.
   *
   * @param planner_id Stable planner identifier.
   * @return True when the planner is registered.
   */
  [[nodiscard]] bool Contains(std::string_view planner_id) const;

  /**
   * @brief Looks up registered planner capabilities.
   *
   * @param planner_id Stable planner identifier.
   * @return Planner capabilities when registered.
   */
  [[nodiscard]] std::optional<PlannerCapabilities> Capabilities(std::string_view planner_id) const;

  /**
   * @brief Enumerates all registered planner records.
   *
   * @return Snapshot of all registered planner records.
   */
  [[nodiscard]] std::vector<PlannerRecord> EnumeratePlanners() const;

  /**
   * @brief Returns the number of registered planners.
   *
   * @return Registered planner count.
   */
  [[nodiscard]] std::size_t PlannerCount() const;

private:
  /**
   * @brief Converts a planner identifier into a map key.
   *
   * @param planner_id Stable planner identifier.
   * @return String key.
   */
  [[nodiscard]] static std::string MakeKey(std::string_view planner_id);

  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, PlannerRecord> planners_;
};

} // namespace humanoid::planner
