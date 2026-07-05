#include <humanoid/planner/PlannerFactory.h>
#include <humanoid/planner/RuleBasedPlanner.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

namespace {

void Require(bool condition) {
  if (!condition) {
    std::abort();
  }
}

[[nodiscard]] humanoid::planner::Goal MakeGoal(humanoid::planner::GoalType type,
                                               humanoid::planner::GoalPriority priority) {
  humanoid::planner::Goal goal;
  goal.id = 101U;
  goal.type = type;
  goal.priority = priority;
  goal.description = "deterministic planner test goal";
  return goal;
}

[[nodiscard]] humanoid::planner::PlanningRequest
MakeRequest(humanoid::planner::GoalType type, humanoid::planner::GoalPriority priority) {
  humanoid::planner::PlanningRequest request;
  request.goal = MakeGoal(type, priority);
  request.robotCapabilities.supportsLifecycle = true;
  request.robotCapabilities.supportsConnectionManagement = true;
  request.robotCapabilities.supportsStateFeedback = true;
  return request;
}

void VerifyDefaultRuleMatchingAndExecutableTree() {
  humanoid::planner::RuleBasedPlanner planner;

  humanoid::planner::PlanningResult result = planner.Plan(MakeRequest(
      humanoid::planner::GoalType::Inspection, humanoid::planner::GoalPriority::Normal));

  Require(result.status == humanoid::planner::GoalStatus::Planned);
  Require(result.hasMission());
  Require(result.hasBehaviorTree());
  Require(result.mission->isValid());
  Require(result.mission->metadata.at("rule_id") == "rule.inspection.default");
  Require(result.mission->steps.size() == 1U);
  Require(result.mission->steps.front().command.type == humanoid::core::CommandType::Custom);
  Require(result.mission->steps.front().command.priority ==
          humanoid::core::CommandPriority::Normal);
  Require(planner.ValidatePlan(result).isOk());

  Require(result.behaviorTree->Initialize() == humanoid::bt::BTStatus::Idle);
  Require(result.behaviorTree->Tick() == humanoid::bt::BTStatus::Success);
}

void VerifyPrioritySelectsMostSpecificRule() {
  humanoid::planner::RuleBasedPlanner planner{humanoid::planner::RuleBasedPlanner::RuleSet{
      humanoid::planner::RuleBasedPlannerRule{"rule.command.low",
                                              humanoid::planner::GoalType::Command,
                                              humanoid::planner::GoalPriority::Low,
                                              {humanoid::core::CommandType::Custom},
                                              "Low Command Plan",
                                              false},
      humanoid::planner::RuleBasedPlannerRule{"rule.command.high",
                                              humanoid::planner::GoalType::Command,
                                              humanoid::planner::GoalPriority::High,
                                              {humanoid::core::CommandType::Stop},
                                              "High Command Plan",
                                              false},
      humanoid::planner::RuleBasedPlannerRule{"rule.fallback",
                                              humanoid::planner::GoalType::Custom,
                                              humanoid::planner::GoalPriority::Low,
                                              {humanoid::core::CommandType::Custom},
                                              "Fallback",
                                              true}}};

  humanoid::planner::PlanningResult normal_result = planner.Plan(
      MakeRequest(humanoid::planner::GoalType::Command, humanoid::planner::GoalPriority::Normal));
  Require(normal_result.hasMission());
  Require(normal_result.mission->metadata.at("rule_id") == "rule.command.low");
  Require(normal_result.mission->steps.front().command.type == humanoid::core::CommandType::Custom);

  humanoid::planner::PlanningResult high_result = planner.Plan(
      MakeRequest(humanoid::planner::GoalType::Command, humanoid::planner::GoalPriority::High));
  Require(high_result.hasMission());
  Require(high_result.mission->metadata.at("rule_id") == "rule.command.high");
  Require(high_result.mission->steps.front().command.type == humanoid::core::CommandType::Stop);
  Require(high_result.mission->steps.front().command.priority ==
          humanoid::core::CommandPriority::High);
}

void VerifyFallbackRule() {
  humanoid::planner::RuleBasedPlanner planner{humanoid::planner::RuleBasedPlanner::RuleSet{
      humanoid::planner::RuleBasedPlannerRule{"rule.command.only",
                                              humanoid::planner::GoalType::Command,
                                              humanoid::planner::GoalPriority::Low,
                                              {humanoid::core::CommandType::Stop},
                                              "Command Only",
                                              false},
      humanoid::planner::RuleBasedPlannerRule{"rule.fallback.explicit",
                                              humanoid::planner::GoalType::Custom,
                                              humanoid::planner::GoalPriority::Low,
                                              {humanoid::core::CommandType::Custom},
                                              "Fallback",
                                              true}}};

  humanoid::planner::PlanningResult result = planner.Plan(
      MakeRequest(humanoid::planner::GoalType::Interaction, humanoid::planner::GoalPriority::Low));

  Require(result.status == humanoid::planner::GoalStatus::Planned);
  Require(result.hasMission());
  Require(result.mission->metadata.at("rule_id") == "rule.fallback.explicit");
  Require(!result.diagnostics.empty());
  Require(result.diagnostics.front().severity ==
          humanoid::planner::PlanningDiagnosticSeverity::Warning);
}

void VerifyInvalidRequestAndCancellation() {
  humanoid::planner::RuleBasedPlanner planner;

  humanoid::planner::PlanningResult rejected = planner.Plan(humanoid::planner::PlanningRequest{});
  Require(rejected.status == humanoid::planner::GoalStatus::Rejected);
  Require(!rejected.isSuccess());

  Require(planner.CancelPlan().isOk());
  humanoid::planner::PlanningResult cancelled = planner.Plan(
      MakeRequest(humanoid::planner::GoalType::Mission, humanoid::planner::GoalPriority::Normal));
  Require(cancelled.status == humanoid::planner::GoalStatus::Cancelled);
}

void VerifyFactoryIntegration() {
  humanoid::planner::PlannerFactory factory;
  humanoid::planner::RuleBasedPlanner planner;
  const humanoid::planner::PlannerCapabilities capabilities = planner.GetCapabilities();

  Require(
      factory
          .RegisterPlanner(capabilities,
                           []() { return std::make_unique<humanoid::planner::RuleBasedPlanner>(); })
          .isOk());

  humanoid::planner::PlannerCreationResult created = factory.CreatePlanner(capabilities.plannerId);
  Require(created.status.isOk());
  Require(static_cast<bool>(created.planner));
  Require(created.planner->GetCapabilities().kind == humanoid::planner::PlannerKind::Rule);
  Require(factory.DestroyPlanner(created.planner).isOk());
}

} // namespace

int main() {
  VerifyDefaultRuleMatchingAndExecutableTree();
  VerifyPrioritySelectsMostSpecificRule();
  VerifyFallbackRule();
  VerifyInvalidRequestAndCancellation();
  VerifyFactoryIntegration();
  return 0;
}
