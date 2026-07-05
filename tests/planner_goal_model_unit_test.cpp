#include <humanoid/planner/Goal.h>
#include <humanoid/planner/PlanningRequest.h>
#include <humanoid/planner/PlanningResult.h>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace {

void VerifyGoalDefaultsAndValidation() {
  humanoid::planner::Goal goal;

  assert(goal.id == humanoid::planner::kInvalidGoalId);
  assert(goal.type == humanoid::planner::GoalType::Custom);
  assert(goal.priority == humanoid::planner::GoalPriority::Normal);
  assert(goal.status == humanoid::planner::GoalStatus::Pending);
  assert(!goal.isValid());
  assert(!goal.isTerminal());

  goal.id = 42U;
  goal.type = humanoid::planner::GoalType::Mission;
  goal.description = "Inspect the charging area";
  goal.priority = humanoid::planner::GoalPriority::High;
  goal.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  goal.constraints.emplace("maximum_duration_ms", std::int64_t{30000});
  goal.context.emplace("operator_present", true);
  goal.metadata.emplace("source", "unit-test");

  assert(goal.isValid());
  assert(humanoid::planner::toString(goal.type) == "Mission");
  assert(humanoid::planner::toString(goal.priority) == "High");

  goal.status = humanoid::planner::GoalStatus::Planned;
  assert(goal.isTerminal());
}

void VerifyPlanningRequest() {
  humanoid::planner::PlanningRequest request;

  assert(!request.isValid());

  request.goal.id = 7U;
  request.goal.description = "Wave to operator";
  request.robotCapabilities.supportsLifecycle = true;
  request.robotCapabilities.supportsStateFeedback = true;

  assert(request.isValid());
  assert(request.robotCapabilities.supportsLifecycle);
  assert(request.robotCapabilities.supportsStateFeedback);
}

void VerifyPlanningResultMoveOnlyOutput() {
  humanoid::planner::PlanningResult result;

  assert(!result.hasMission());
  assert(!result.hasBehaviorTree());
  assert(!result.isSuccess());

  humanoid::mission::Mission mission;
  mission.id = 99U;
  mission.name = "Generated mission";
  result.mission = std::move(mission);
  result.status = humanoid::planner::GoalStatus::Planned;
  result.diagnostics.push_back(
      humanoid::planner::PlanningDiagnostic{humanoid::planner::PlanningDiagnosticSeverity::Info,
                                            "planner.generated", "Generated a mission plan"});

  assert(result.hasMission());
  assert(!result.hasBehaviorTree());
  assert(result.isSuccess());
  assert(result.diagnostics.size() == 1U);

  humanoid::planner::PlanningResult moved{std::move(result)};
  assert(moved.hasMission());
  assert(moved.isSuccess());
}

} // namespace

int main() {
  VerifyGoalDefaultsAndValidation();
  VerifyPlanningRequest();
  VerifyPlanningResultMoveOnlyOutput();
  return 0;
}
