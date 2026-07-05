/**
 * @file main.cpp
 * @brief Runs hardware-free planning pipeline examples.
 */

#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <humanoid/bt/BehaviorTreeFactory.h>
#include <humanoid/bt/BehaviorTreeRuntime.h>
#include <humanoid/common/Status.hpp>
#include <humanoid/core/CommandType.h>
#include <humanoid/logging/Logger.hpp>
#include <humanoid/mission/Mission.h>
#include <humanoid/mission/MissionStep.h>
#include <humanoid/planner/GoalPriority.h>
#include <humanoid/planner/GoalType.h>
#include <humanoid/planner/PlanningPipeline.h>
#include <humanoid/planner/RuleBasedPlanner.h>
#include <humanoid/runtime/Blackboard.h>
#include <humanoid/runtime/ExecutionState.h>
#include <humanoid/runtime/ResourceManager.h>
#include <humanoid/runtime/RuntimeScheduler.h>

namespace {

class ConsoleLogger final : public humanoid::logging::ILogger {
public:
  humanoid::common::Status log(const humanoid::logging::LogMessage& message) override {
    std::cout << "log[" << humanoid::logging::toString(message.level()) << "] "
              << message.component() << ": " << message.text() << '\n';
    return humanoid::common::Status::ok();
  }

  [[nodiscard]] bool isEnabled(humanoid::logging::LogLevel) const noexcept override { return true; }
};

struct PlanningExample final {
  std::string key;
  std::string title;
  std::string description;
  humanoid::planner::GoalType goalType{humanoid::planner::GoalType::Custom};
  humanoid::planner::GoalPriority priority{humanoid::planner::GoalPriority::Normal};
  std::vector<humanoid::core::CommandType> commands;
};

[[nodiscard]] std::vector<PlanningExample> MakeExamples() {
  return {
      PlanningExample{"greeting",
                      "Greeting",
                      "Wave to audience",
                      humanoid::planner::GoalType::Interaction,
                      humanoid::planner::GoalPriority::Normal,
                      {humanoid::core::CommandType::Stand, humanoid::core::CommandType::HandOpen,
                       humanoid::core::CommandType::HandClose}},
      PlanningExample{"flag-ceremony",
                      "Flag Ceremony",
                      "Perform a respectful flag ceremony",
                      humanoid::planner::GoalType::Mission,
                      humanoid::planner::GoalPriority::High,
                      {humanoid::core::CommandType::Stand, humanoid::core::CommandType::PlayAudio,
                       humanoid::core::CommandType::HandOpen,
                       humanoid::core::CommandType::StopAudio}},
      PlanningExample{"inspection",
                      "Inspection",
                      "Inspect the demonstration area",
                      humanoid::planner::GoalType::Inspection,
                      humanoid::planner::GoalPriority::Normal,
                      {humanoid::core::CommandType::Move, humanoid::core::CommandType::Rotate,
                       humanoid::core::CommandType::Stop}},
      PlanningExample{"stage-demo",
                      "Stage Demo",
                      "Run a stage presentation routine",
                      humanoid::planner::GoalType::BehaviorTree,
                      humanoid::planner::GoalPriority::High,
                      {humanoid::core::CommandType::Stand, humanoid::core::CommandType::Move,
                       humanoid::core::CommandType::Rotate, humanoid::core::CommandType::Stop,
                       humanoid::core::CommandType::HandOpen,
                       humanoid::core::CommandType::PlayAudio,
                       humanoid::core::CommandType::StopAudio}}};
}

[[nodiscard]] humanoid::planner::RuleBasedPlanner::RuleSet
MakeRules(const std::vector<PlanningExample>& examples) {
  humanoid::planner::RuleBasedPlanner::RuleSet rules;
  rules.reserve(examples.size() + 1U);

  for (const PlanningExample& example : examples) {
    rules.push_back(humanoid::planner::RuleBasedPlannerRule{
        "example." + example.key, example.goalType, humanoid::planner::GoalPriority::Low,
        example.commands, example.title + " Plan", false});
  }

  rules.push_back(humanoid::planner::RuleBasedPlannerRule{"example.fallback",
                                                          humanoid::planner::GoalType::Custom,
                                                          humanoid::planner::GoalPriority::Low,
                                                          {humanoid::core::CommandType::Custom},
                                                          "Fallback Planning Example",
                                                          true});
  return rules;
}

[[nodiscard]] humanoid::planner::PlanningRequest MakeRequest(const PlanningExample& example,
                                                             std::uint64_t id) {
  humanoid::planner::PlanningRequest request;
  request.goal.id = id;
  request.goal.type = example.goalType;
  request.goal.description = example.description;
  request.goal.priority = example.priority;
  request.goal.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  request.goal.metadata.emplace("example", example.key);
  request.robotCapabilities.supportsLifecycle = true;
  request.robotCapabilities.supportsConnectionManagement = true;
  request.robotCapabilities.supportsStateFeedback = true;
  request.robotCapabilities.supportsRobotInformation = true;
  request.robotCapabilities.supportsPeriodicUpdate = true;
  return request;
}

void PrintMission(const humanoid::mission::Mission& mission) {
  std::cout << "mission: " << mission.name << '\n';
  std::cout << "steps:\n";
  for (const humanoid::mission::MissionStep& step : mission.steps) {
    std::cout << "  - " << step.name << " -> " << humanoid::core::toString(step.command.type)
              << '\n';
  }
}

void PrintDiagnostics(const humanoid::planner::PlanningDiagnostics& diagnostics) {
  std::cout << "diagnostics:\n";
  for (const humanoid::planner::PlanningDiagnostic& diagnostic : diagnostics) {
    std::cout << "  - " << diagnostic.code << ": " << diagnostic.message << '\n';
  }
}

[[nodiscard]] bool RunExample(const PlanningExample& example, std::uint64_t id) {
  auto scheduler = std::make_shared<humanoid::runtime::RuntimeScheduler>(
      humanoid::runtime::RuntimeSchedulerOptions{16U, 2U, 16U});
  auto blackboard = std::make_shared<humanoid::runtime::Blackboard>();
  auto resource_manager = std::make_shared<humanoid::runtime::ResourceManager>();
  auto factory = std::make_shared<humanoid::bt::BehaviorTreeFactory>();
  auto behavior_tree_runtime = std::make_shared<humanoid::bt::BehaviorTreeRuntime>(
      scheduler, blackboard, resource_manager, factory);
  auto planner = std::make_shared<humanoid::planner::RuleBasedPlanner>(MakeRules(MakeExamples()));
  auto logger = std::make_shared<ConsoleLogger>();

  humanoid::planner::PlanningPipeline pipeline{planner, {}, behavior_tree_runtime, logger};
  humanoid::planner::PlanningPipelineOptions options;
  options.submitBehaviorTreeToRuntime = true;
  options.runtimeOptions.executionId = id + 10000U;
  options.runtimeOptions.resourceId = "planning/example/robot";
  options.runtimeOptions.resourceTimeout = std::chrono::seconds{1};
  options.runtimeOptions.tickInterval = std::chrono::milliseconds{5};
  options.runtimeOptions.metadata.emplace("example", example.key);

  std::cout << "\n== " << example.title << " ==\n";
  std::cout << "goal: " << example.description << '\n';

  humanoid::planner::PlanningPipelineResult result =
      pipeline.Execute(MakeRequest(example, id), std::move(options));

  if (!result.status.isOk()) {
    std::cerr << "planning failed: " << result.status.message() << '\n';
    PrintDiagnostics(result.planningResult.diagnostics);
    scheduler->Shutdown();
    return false;
  }

  if (result.planningResult.mission.has_value()) {
    PrintMission(result.planningResult.mission.value());
  }
  PrintDiagnostics(result.planningResult.diagnostics);

  if (!result.runtimeHandle.has_value()) {
    std::cerr << "execution failed: runtime handoff was not created\n";
    scheduler->Shutdown();
    return false;
  }

  const humanoid::runtime::RuntimeJobResult runtime_result = result.runtimeHandle->result.get();
  std::cout << "execution: " << runtime_result.message << '\n';
  scheduler->Shutdown();
  return runtime_result.state == humanoid::runtime::ExecutionState::Completed;
}

[[nodiscard]] const PlanningExample* FindExample(const std::vector<PlanningExample>& examples,
                                                 std::string_view key) {
  for (const PlanningExample& example : examples) {
    if (example.key == key) {
      return &example;
    }
  }
  return nullptr;
}

int Run(int argc, char** argv) {
  const std::vector<PlanningExample> examples = MakeExamples();
  const std::string selected = argc > 1 ? std::string{argv[1]} : "all";

  if (selected == "all") {
    std::uint64_t id = 9601U;
    bool success = true;
    for (const PlanningExample& example : examples) {
      success = RunExample(example, id) && success;
      ++id;
    }
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  const PlanningExample* example = FindExample(examples, selected);
  if (example == nullptr) {
    std::cerr << "unknown planning example: " << selected << '\n';
    std::cerr << "available examples: all";
    for (const PlanningExample& candidate : examples) {
      std::cerr << ", " << candidate.key;
    }
    std::cerr << '\n';
    return EXIT_FAILURE;
  }

  return RunExample(*example, 9601U) ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace

int main(int argc, char** argv) {
  try {
    return Run(argc, argv);
  } catch (const std::exception& exception) {
    std::cerr << "planning example failed: " << exception.what() << '\n';
  } catch (...) {
    std::cerr << "planning example failed with an unknown error\n";
  }
  return EXIT_FAILURE;
}
