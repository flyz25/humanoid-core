#include <humanoid/planner/PlannerFactory.h>

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

[[nodiscard]] humanoid::common::Status InternalError(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kInternalError,
                                         std::move(message));
}

} // namespace

PlannerFactory::PlannerFactory() : PlannerFactory(std::make_shared<PlannerRegistry>()) {}

PlannerFactory::PlannerFactory(std::shared_ptr<PlannerRegistry> registry) noexcept
    : registry_(std::move(registry)) {}

humanoid::common::Status PlannerFactory::RegisterPlanner(PlannerCapabilities capabilities,
                                                         PlannerCreator creator) {
  if (!registry_) {
    return FailedPrecondition("planner registry is not available");
  }

  if (!creator) {
    return InvalidArgument("planner creator is empty");
  }

  if (!capabilities.isValid()) {
    return InvalidArgument("planner capabilities are incomplete or invalid");
  }

  const std::string planner_id = capabilities.plannerId;
  std::unique_lock<std::shared_mutex> lock{mutex_};
  if (factories_.find(planner_id) != factories_.end()) {
    return FailedPrecondition("planner creator is already registered");
  }

  const humanoid::common::Status registry_status = registry_->RegisterPlanner(capabilities);
  if (!registry_status.isOk()) {
    return registry_status;
  }

  try {
    factories_.emplace(planner_id, FactoryEntry{std::move(capabilities), std::move(creator), 0U});
  } catch (...) {
    (void)registry_->UnregisterPlanner(planner_id);
    return InternalError("failed to store planner creator");
  }

  return humanoid::common::Status::ok();
}

humanoid::common::Status PlannerFactory::UnregisterPlanner(std::string_view planner_id) {
  if (!registry_) {
    return FailedPrecondition("planner registry is not available");
  }

  if (planner_id.empty()) {
    return InvalidArgument("planner identifier is empty");
  }

  const std::string key = MakeKey(planner_id);
  std::unique_lock<std::shared_mutex> lock{mutex_};
  const auto iterator = factories_.find(key);
  if (iterator == factories_.end()) {
    return FailedPrecondition("planner creator is not registered");
  }

  if (iterator->second.activeInstances > 0U) {
    return FailedPrecondition("planner instances are still active");
  }

  const humanoid::common::Status registry_status = registry_->UnregisterPlanner(key);
  if (!registry_status.isOk()) {
    return registry_status;
  }

  factories_.erase(iterator);
  return humanoid::common::Status::ok();
}

PlannerCreationResult PlannerFactory::CreatePlanner(std::string_view planner_id) {
  if (!registry_) {
    return PlannerCreationResult{FailedPrecondition("planner registry is not available"), nullptr};
  }

  if (planner_id.empty()) {
    return PlannerCreationResult{InvalidArgument("planner identifier is empty"), nullptr};
  }

  const std::string key = MakeKey(planner_id);
  PlannerCreator creator;
  try {
    std::unique_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = factories_.find(key);
    if (iterator == factories_.end()) {
      return PlannerCreationResult{FailedPrecondition("planner creator is not registered"),
                                   nullptr};
    }

    creator = iterator->second.creator;
    ++iterator->second.activeInstances;
  } catch (...) {
    return PlannerCreationResult{InternalError("failed to prepare planner creator"), nullptr};
  }

  const auto release_active_instance = [this, &key]() {
    std::unique_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = factories_.find(key);
    if (iterator != factories_.end() && iterator->second.activeInstances > 0U) {
      --iterator->second.activeInstances;
    }
  };

  std::unique_ptr<IPlanner> planner;
  try {
    planner = creator();
  } catch (...) {
    release_active_instance();
    return PlannerCreationResult{InternalError("planner creator threw an exception"), nullptr};
  }

  if (!planner) {
    release_active_instance();
    return PlannerCreationResult{InternalError("planner creator returned null"), nullptr};
  }

  PlannerCapabilities created_capabilities;
  try {
    created_capabilities = planner->GetCapabilities();
  } catch (...) {
    release_active_instance();
    return PlannerCreationResult{InternalError("planner capability query threw an exception"),
                                 nullptr};
  }

  if (!created_capabilities.isValid() || created_capabilities.plannerId != key) {
    release_active_instance();
    return PlannerCreationResult{
        InternalError("created planner capabilities do not match requested identifier"), nullptr};
  }

  return PlannerCreationResult{humanoid::common::Status::ok(), std::move(planner)};
}

humanoid::common::Status PlannerFactory::DestroyPlanner(std::unique_ptr<IPlanner>& planner) {
  if (!planner) {
    return InvalidArgument("planner instance is null");
  }

  PlannerCapabilities capabilities;
  try {
    capabilities = planner->GetCapabilities();
  } catch (...) {
    return InternalError("planner capability query threw an exception");
  }

  if (capabilities.plannerId.empty()) {
    return InvalidArgument("planner identifier is empty");
  }

  {
    std::unique_lock<std::shared_mutex> lock{mutex_};
    const auto iterator = factories_.find(capabilities.plannerId);
    if (iterator == factories_.end() || iterator->second.activeInstances == 0U) {
      return FailedPrecondition("planner instance is not tracked by this factory");
    }

    --iterator->second.activeInstances;
  }

  planner.reset();
  return humanoid::common::Status::ok();
}

std::vector<PlannerRecord> PlannerFactory::EnumeratePlanners() const {
  if (!registry_) {
    return {};
  }

  return registry_->EnumeratePlanners();
}

std::size_t PlannerFactory::RegisteredPlannerCount() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return factories_.size();
}

std::shared_ptr<PlannerRegistry> PlannerFactory::Registry() const noexcept { return registry_; }

std::string PlannerFactory::MakeKey(std::string_view planner_id) { return std::string{planner_id}; }

} // namespace humanoid::planner
