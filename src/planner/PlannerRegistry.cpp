#include <humanoid/planner/PlannerRegistry.h>

#include <mutex>
#include <utility>

namespace humanoid::planner {

namespace {

[[nodiscard]] humanoid::common::Status InvalidArgument(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kInvalidArgument,
                                         std::move(message));
}

[[nodiscard]] humanoid::common::Status FailedPrecondition(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kFailedPrecondition,
                                         std::move(message));
}

} // namespace

humanoid::common::Status PlannerRegistry::RegisterPlanner(PlannerCapabilities capabilities) {
  if (!capabilities.isValid()) {
    return InvalidArgument("planner capabilities are incomplete or invalid");
  }

  const std::string planner_id = capabilities.plannerId;
  std::unique_lock<std::shared_mutex> lock{mutex_};
  const auto [unused_iterator, inserted] =
      planners_.emplace(planner_id, PlannerRecord{std::move(capabilities)});
  (void)unused_iterator;

  if (!inserted) {
    return FailedPrecondition("planner is already registered");
  }

  return humanoid::common::Status::ok();
}

humanoid::common::Status PlannerRegistry::UnregisterPlanner(std::string_view planner_id) {
  if (planner_id.empty()) {
    return InvalidArgument("planner identifier is empty");
  }

  std::unique_lock<std::shared_mutex> lock{mutex_};
  if (planners_.erase(MakeKey(planner_id)) == 0U) {
    return FailedPrecondition("planner is not registered");
  }

  return humanoid::common::Status::ok();
}

bool PlannerRegistry::Contains(std::string_view planner_id) const {
  if (planner_id.empty()) {
    return false;
  }

  std::shared_lock<std::shared_mutex> lock{mutex_};
  return planners_.find(MakeKey(planner_id)) != planners_.end();
}

std::optional<PlannerCapabilities>
PlannerRegistry::Capabilities(std::string_view planner_id) const {
  if (planner_id.empty()) {
    return std::nullopt;
  }

  std::shared_lock<std::shared_mutex> lock{mutex_};
  const auto iterator = planners_.find(MakeKey(planner_id));
  if (iterator == planners_.end()) {
    return std::nullopt;
  }

  return iterator->second.capabilities;
}

std::vector<PlannerRecord> PlannerRegistry::EnumeratePlanners() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  std::vector<PlannerRecord> records;
  records.reserve(planners_.size());

  for (const auto& [planner_id, record] : planners_) {
    (void)planner_id;
    records.push_back(record);
  }

  return records;
}

std::size_t PlannerRegistry::PlannerCount() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return planners_.size();
}

std::string PlannerRegistry::MakeKey(std::string_view planner_id) {
  return std::string{planner_id};
}

} // namespace humanoid::planner
