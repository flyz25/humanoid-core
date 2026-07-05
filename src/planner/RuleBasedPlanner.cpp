#include <humanoid/planner/RuleBasedPlanner.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include <humanoid/bt/ActionNode.h>
#include <humanoid/bt/BTStatus.h>
#include <humanoid/bt/BehaviorTree.h>
#include <humanoid/bt/SequenceNode.h>
#include <humanoid/core/Command.h>
#include <humanoid/core/CommandPriority.h>
#include <humanoid/mission/Mission.h>
#include <humanoid/mission/MissionStep.h>

namespace humanoid::planner {

namespace {

[[nodiscard]] humanoid::common::Status InvalidArgument(std::string message) {
  return humanoid::common::Status::error(humanoid::common::StatusCode::kInvalidArgument,
                                         std::move(message));
}

[[nodiscard]] int PriorityRank(GoalPriority priority) noexcept {
  switch (priority) {
  case GoalPriority::Low:
    return 0;
  case GoalPriority::Normal:
    return 1;
  case GoalPriority::High:
    return 2;
  case GoalPriority::Critical:
    return 3;
  }

  return -1;
}

[[nodiscard]] humanoid::core::CommandPriority ToCommandPriority(GoalPriority priority) noexcept {
  switch (priority) {
  case GoalPriority::Low:
    return humanoid::core::CommandPriority::Low;
  case GoalPriority::Normal:
    return humanoid::core::CommandPriority::Normal;
  case GoalPriority::High:
    return humanoid::core::CommandPriority::High;
  case GoalPriority::Critical:
    return humanoid::core::CommandPriority::Critical;
  }

  return humanoid::core::CommandPriority::Normal;
}

[[nodiscard]] std::uint64_t DerivedId(std::uint64_t seed, std::uint64_t offset) noexcept {
  if (seed != 0U && seed <= std::numeric_limits<std::uint64_t>::max() - offset) {
    return seed + offset;
  }

  return offset == 0U ? 1U : offset;
}

[[nodiscard]] std::uint64_t CommandSeed(std::uint64_t goal_id) noexcept {
  constexpr std::uint64_t kCommandIdScale{1000U};
  if (goal_id != 0U && goal_id <= std::numeric_limits<std::uint64_t>::max() / kCommandIdScale) {
    return goal_id * kCommandIdScale;
  }

  return goal_id;
}

[[nodiscard]] humanoid::core::Command MakeCommand(const Goal& goal,
                                                  const RuleBasedPlannerRule& rule,
                                                  humanoid::core::CommandType command_type,
                                                  std::uint64_t step_index) {
  humanoid::core::Command command;
  command.id = DerivedId(CommandSeed(goal.id), step_index);
  command.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  command.type = command_type;
  command.priority = ToCommandPriority(goal.priority);
  command.payload.emplace("goal_type", std::string{toString(goal.type)});
  command.payload.emplace("goal_description", goal.description);
  command.payload.emplace("rule_id", rule.ruleId);
  command.metadata.emplace("planner", "RuleBasedPlanner");
  command.metadata.emplace("goal_id", std::to_string(goal.id));
  command.metadata.emplace("rule_id", rule.ruleId);
  return command;
}

[[nodiscard]] humanoid::mission::Mission MakeMission(const PlanningRequest& request,
                                                     const RuleBasedPlannerRule& rule) {
  humanoid::mission::Mission mission;
  mission.id = request.goal.id;
  mission.name = rule.missionName + ": " + request.goal.description;
  mission.description =
      "Deterministic rule-based plan generated from goal " + std::to_string(request.goal.id);
  mission.version = "1";
  mission.author = "RuleBasedPlanner";
  mission.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  mission.metadata.emplace("planner", "RuleBasedPlanner");
  mission.metadata.emplace("rule_id", rule.ruleId);
  mission.metadata.emplace("goal_type", std::string{toString(request.goal.type)});

  mission.steps.reserve(rule.commandSequence.size());
  std::uint64_t step_index = 1U;
  for (const humanoid::core::CommandType command_type : rule.commandSequence) {
    humanoid::mission::MissionStep step;
    step.id = step_index;
    step.name = std::string{"Rule step "} + std::to_string(step_index) + " " +
                std::string{humanoid::core::toString(command_type)};
    step.command = MakeCommand(request.goal, rule, command_type, step_index);
    step.metadata.emplace("planner", "RuleBasedPlanner");
    step.metadata.emplace("rule_id", rule.ruleId);
    mission.steps.push_back(std::move(step));
    ++step_index;
  }

  return mission;
}

[[nodiscard]] std::unique_ptr<humanoid::bt::BehaviorTree>
MakeBehaviorTree(const RuleBasedPlannerRule& rule) {
  humanoid::bt::BTChildren children;
  children.reserve(rule.commandSequence.size());

  for (const humanoid::core::CommandType command_type : rule.commandSequence) {
    children.push_back(std::make_unique<humanoid::bt::ActionNode>(
        std::string{"Planned "} + std::string{humanoid::core::toString(command_type)},
        [](humanoid::bt::BTContext&) { return humanoid::bt::BTStatus::Success; }));
  }

  auto root = std::make_unique<humanoid::bt::SequenceNode>(
      std::string{"RuleBasedPlanner "} + rule.ruleId, humanoid::bt::SequenceMemoryPolicy::Stateless,
      std::move(children));
  return std::make_unique<humanoid::bt::BehaviorTree>(std::move(root));
}

[[nodiscard]] std::optional<RuleBasedPlannerRule> SelectRule(const RuleBasedPlanner::RuleSet& rules,
                                                             const Goal& goal) {
  std::optional<RuleBasedPlannerRule> fallback;
  std::optional<RuleBasedPlannerRule> selected;
  int selected_priority = -1;

  for (const RuleBasedPlannerRule& rule : rules) {
    if (!rule.isValid()) {
      continue;
    }

    if (rule.fallback) {
      if (!fallback.has_value()) {
        fallback = rule;
      }
      continue;
    }

    if (rule.goalType != goal.type) {
      continue;
    }

    const int rule_priority = PriorityRank(rule.minimumPriority);
    if (PriorityRank(goal.priority) < rule_priority) {
      continue;
    }

    if (!selected.has_value() || rule_priority > selected_priority) {
      selected = rule;
      selected_priority = rule_priority;
    }
  }

  if (selected.has_value()) {
    return selected;
  }

  return fallback;
}

void AddDiagnostic(PlanningResult& result, PlanningDiagnosticSeverity severity, std::string code,
                   std::string message) {
  result.diagnostics.push_back(PlanningDiagnostic{severity, std::move(code), std::move(message)});
}

} // namespace

RuleBasedPlanner::RuleBasedPlanner() : RuleBasedPlanner(DefaultRules()) {}

RuleBasedPlanner::RuleBasedPlanner(RuleSet rules) : capabilities_(DefaultCapabilities()) {
  rules_.reserve(rules.size() + 1U);
  bool has_fallback = false;
  for (RuleBasedPlannerRule& rule : rules) {
    if (!rule.isValid()) {
      continue;
    }
    has_fallback = has_fallback || rule.fallback;
    rules_.push_back(std::move(rule));
  }

  if (!has_fallback) {
    rules_.push_back(DefaultFallbackRule());
  }
}

PlanningResult RuleBasedPlanner::Plan(const PlanningRequest& request) {
  PlanningResult result;

  if (cancellationRequested_.exchange(false)) {
    result.status = GoalStatus::Cancelled;
    AddDiagnostic(result, PlanningDiagnosticSeverity::Warning, "planner.cancelled",
                  "planning was cancelled before rule evaluation");
    return result;
  }

  if (!request.isValid()) {
    result.status = GoalStatus::Rejected;
    AddDiagnostic(result, PlanningDiagnosticSeverity::Error, "planner.invalid_request",
                  "planning request does not contain a valid goal");
    return result;
  }

  std::optional<RuleBasedPlannerRule> selected_rule;
  {
    std::shared_lock<std::shared_mutex> lock{mutex_};
    selected_rule = SelectRule(rules_, request.goal);
  }

  if (!selected_rule.has_value()) {
    result.status = GoalStatus::Failed;
    AddDiagnostic(result, PlanningDiagnosticSeverity::Error, "planner.no_rule",
                  "no valid rule or fallback rule is available");
    return result;
  }

  if (cancellationRequested_.exchange(false)) {
    result.status = GoalStatus::Cancelled;
    AddDiagnostic(result, PlanningDiagnosticSeverity::Warning, "planner.cancelled",
                  "planning was cancelled after rule selection");
    return result;
  }

  result.mission = MakeMission(request, selected_rule.value());
  result.behaviorTree = MakeBehaviorTree(selected_rule.value());
  result.status = GoalStatus::Planned;
  AddDiagnostic(result,
                selected_rule->fallback ? PlanningDiagnosticSeverity::Warning
                                        : PlanningDiagnosticSeverity::Info,
                selected_rule->fallback ? "planner.fallback_rule" : "planner.rule_matched",
                std::string{"selected rule "} + selected_rule->ruleId);
  return result;
}

humanoid::common::Status RuleBasedPlanner::ValidatePlan(const PlanningResult& result) const {
  if (result.status != GoalStatus::Planned) {
    return InvalidArgument("planning result is not in Planned status");
  }
  if (!result.hasMission()) {
    return InvalidArgument("planning result does not contain a mission");
  }
  if (!result.mission->isValid()) {
    return InvalidArgument("planning result mission is invalid");
  }
  if (!result.hasBehaviorTree()) {
    return InvalidArgument("planning result does not contain a behavior tree");
  }

  return humanoid::common::Status::ok();
}

humanoid::common::Status RuleBasedPlanner::CancelPlan() {
  cancellationRequested_.store(true);
  return humanoid::common::Status::ok();
}

PlannerCapabilities RuleBasedPlanner::GetCapabilities() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return capabilities_;
}

std::size_t RuleBasedPlanner::RuleCount() const {
  std::shared_lock<std::shared_mutex> lock{mutex_};
  return rules_.size();
}

RuleBasedPlanner::RuleSet RuleBasedPlanner::DefaultRules() {
  return RuleSet{RuleBasedPlannerRule{"rule.command.critical-stop",
                                      GoalType::Command,
                                      GoalPriority::Critical,
                                      {humanoid::core::CommandType::Stop},
                                      "Critical Command Plan",
                                      false},
                 RuleBasedPlannerRule{"rule.mission.default",
                                      GoalType::Mission,
                                      GoalPriority::Low,
                                      {humanoid::core::CommandType::Custom},
                                      "Mission Plan",
                                      false},
                 RuleBasedPlannerRule{"rule.behavior-tree.default",
                                      GoalType::BehaviorTree,
                                      GoalPriority::Low,
                                      {humanoid::core::CommandType::Custom},
                                      "Behavior Tree Plan",
                                      false},
                 RuleBasedPlannerRule{"rule.command.default",
                                      GoalType::Command,
                                      GoalPriority::Low,
                                      {humanoid::core::CommandType::Custom},
                                      "Command Plan",
                                      false},
                 RuleBasedPlannerRule{"rule.inspection.default",
                                      GoalType::Inspection,
                                      GoalPriority::Low,
                                      {humanoid::core::CommandType::Custom},
                                      "Inspection Plan",
                                      false},
                 RuleBasedPlannerRule{"rule.interaction.default",
                                      GoalType::Interaction,
                                      GoalPriority::Low,
                                      {humanoid::core::CommandType::Custom},
                                      "Interaction Plan",
                                      false},
                 DefaultFallbackRule()};
}

RuleBasedPlannerRule RuleBasedPlanner::DefaultFallbackRule() {
  return RuleBasedPlannerRule{"rule.fallback.default", GoalType::Custom,
                              GoalPriority::Low,       {humanoid::core::CommandType::Custom},
                              "Fallback Plan",         true};
}

PlannerCapabilities RuleBasedPlanner::DefaultCapabilities() {
  PlannerCapabilities capabilities;
  capabilities.plannerId = "humanoid.planner.rule_based";
  capabilities.name = "Rule-Based Planner";
  capabilities.description = "Deterministic static planner for framework-owned goals";
  capabilities.kind = PlannerKind::Rule;
  capabilities.supportedGoalTypes = {GoalType::Mission,     GoalType::BehaviorTree,
                                     GoalType::Command,     GoalType::Inspection,
                                     GoalType::Interaction, GoalType::Custom};
  capabilities.supportsMissionOutput = true;
  capabilities.supportsBehaviorTreeOutput = true;
  capabilities.supportsPlanValidation = true;
  capabilities.supportsCancellation = true;
  return capabilities;
}

} // namespace humanoid::planner
