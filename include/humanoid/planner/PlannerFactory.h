#pragma once

/**
 * @file PlannerFactory.h
 * @brief Defines a thread-safe factory for registered planner creators.
 */

#include <cstddef>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <humanoid/common/Status.hpp>
#include <humanoid/planner/IPlanner.h>
#include <humanoid/planner/PlannerRegistry.h>

namespace humanoid::planner {

/**
 * @brief Result returned by planner creation operations.
 */
struct PlannerCreationResult final {
  /**
   * @brief Operation status.
   */
  humanoid::common::Status status{};

  /**
   * @brief Created planner instance when status is successful.
   */
  std::unique_ptr<IPlanner> planner{};
};

/**
 * @brief Thread-safe factory for planner instance creation and destruction.
 *
 * `PlannerFactory` stores creator callables and delegates capability discovery
 * to an injected `PlannerRegistry`. It owns no global state and does not
 * implement any concrete rule, LLM, symbolic, or vendor planner.
 */
class PlannerFactory final {
public:
  /**
   * @brief Callable used to create a planner instance.
   */
  using PlannerCreator = std::function<std::unique_ptr<IPlanner>()>;

  /**
   * @brief Constructs a factory with an internally owned registry.
   */
  PlannerFactory();

  /**
   * @brief Constructs a factory using an injected registry.
   *
   * @param registry Registry used for capability discovery.
   */
  explicit PlannerFactory(std::shared_ptr<PlannerRegistry> registry) noexcept;

  /**
   * @brief Destroys the planner factory.
   */
  ~PlannerFactory() = default;

  PlannerFactory(const PlannerFactory&) = delete;
  PlannerFactory& operator=(const PlannerFactory&) = delete;
  PlannerFactory(PlannerFactory&&) = delete;
  PlannerFactory& operator=(PlannerFactory&&) = delete;

  /**
   * @brief Registers capabilities and a creator for a planner type.
   *
   * @param capabilities Planner capability declaration.
   * @param creator Callable that returns a new planner instance.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status RegisterPlanner(PlannerCapabilities capabilities,
                                                         PlannerCreator creator);

  /**
   * @brief Unregisters a planner type.
   *
   * Unregistration fails while instances created by this factory are still
   * active.
   *
   * @param planner_id Stable planner identifier.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status UnregisterPlanner(std::string_view planner_id);

  /**
   * @brief Creates a planner instance.
   *
   * @param planner_id Stable planner identifier.
   * @return Creation result containing status and optional planner instance.
   */
  [[nodiscard]] PlannerCreationResult CreatePlanner(std::string_view planner_id);

  /**
   * @brief Destroys a planner instance created by this factory.
   *
   * @param planner Planner instance to destroy.
   * @return Operation status.
   */
  [[nodiscard]] humanoid::common::Status DestroyPlanner(std::unique_ptr<IPlanner>& planner);

  /**
   * @brief Enumerates registered planner capability records.
   *
   * @return Snapshot of registered planner records.
   */
  [[nodiscard]] std::vector<PlannerRecord> EnumeratePlanners() const;

  /**
   * @brief Returns the number of registered planner creators.
   *
   * @return Registered creator count.
   */
  [[nodiscard]] std::size_t RegisteredPlannerCount() const;

  /**
   * @brief Returns the injected registry used by this factory.
   *
   * @return Shared planner registry.
   */
  [[nodiscard]] std::shared_ptr<PlannerRegistry> Registry() const noexcept;

private:
  struct FactoryEntry final {
    PlannerCapabilities capabilities{};
    PlannerCreator creator{};
    std::size_t activeInstances{0};
  };

  /**
   * @brief Converts a planner identifier into a map key.
   *
   * @param planner_id Stable planner identifier.
   * @return String key.
   */
  [[nodiscard]] static std::string MakeKey(std::string_view planner_id);

  std::shared_ptr<PlannerRegistry> registry_;
  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, FactoryEntry> factories_;
};

} // namespace humanoid::planner
