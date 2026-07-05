/**
 * @file main.cpp
 * @brief Runs a greeting behavior tree through the shared runtime.
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

int RunGreeting() {
  humanoid::examples::BehaviorTreeExampleRuntime runtime;
  humanoid::bt::BTChildren children;
  children.push_back(std::make_unique<humanoid::bt::CommandNode>(
      "StandForGreeting", runtime.Dispatcher(),
      humanoid::examples::MakeCommand(101U, humanoid::core::CommandType::Stand)));
  children.push_back(std::make_unique<humanoid::bt::ActionNode>(
      "DeliverGreeting", [&runtime](humanoid::bt::BTContext&) {
        runtime.Print("greeting: Welcome to humanoid-core");
        return humanoid::bt::BTStatus::Success;
      }));

  auto root = std::make_unique<humanoid::bt::SequenceNode>(
      "GreetingSequence", humanoid::bt::SequenceMemoryPolicy::Memory, std::move(children));
  return runtime.Run("Greeting", 81001U, std::move(root)) ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace

int main() {
  try {
    return RunGreeting();
  } catch (const std::exception& exception) {
    std::cerr << "Greeting example failed: " << exception.what() << '\n';
  } catch (...) {
    std::cerr << "Greeting example failed with an unknown error\n";
  }
  return EXIT_FAILURE;
}
