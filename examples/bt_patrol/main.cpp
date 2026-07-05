/**
 * @file main.cpp
 * @brief Runs a patrol behavior tree through the shared runtime.
 */

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>

#include <common/BehaviorTreeExampleRuntime.hpp>
#include <humanoid/bt/ActionNode.h>
#include <humanoid/bt/CommandNode.h>
#include <humanoid/bt/CompositeNode.h>
#include <humanoid/bt/SequenceNode.h>
#include <humanoid/core/CommandType.h>

namespace {

int RunPatrol() {
  humanoid::examples::BehaviorTreeExampleRuntime runtime;
  humanoid::bt::BTChildren patrol;
  patrol.push_back(std::make_unique<humanoid::bt::CommandNode>(
      "MoveToInspectionPoint", runtime.Dispatcher(),
      humanoid::examples::MakeMoveCommand(301U, 0.2, 0.0, 0.0)));
  patrol.push_back(std::make_unique<humanoid::bt::ActionNode>(
      "ObserveInspectionPoint", [&runtime](humanoid::bt::BTContext&) {
        runtime.Print("patrol: inspection point observed");
        return humanoid::bt::BTStatus::Success;
      }));
  patrol.push_back(std::make_unique<humanoid::bt::CommandNode>(
      "StopAtInspectionPoint", runtime.Dispatcher(),
      humanoid::examples::MakeCommand(302U, humanoid::core::CommandType::Stop)));

  auto root = std::make_unique<humanoid::bt::SequenceNode>(
      "PatrolSequence", humanoid::bt::SequenceMemoryPolicy::Memory, std::move(patrol));
  return runtime.Run("Patrol", 84001U, std::move(root)) ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace

int main() {
  try {
    return RunPatrol();
  } catch (const std::exception& exception) {
    std::cerr << "Patrol example failed: " << exception.what() << '\n';
  } catch (...) {
    std::cerr << "Patrol example failed with an unknown error\n";
  }
  return EXIT_FAILURE;
}
