/**
 * @file main.cpp
 * @brief Runs a flag ceremony behavior tree through the shared runtime.
 */

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>

#include <common/BehaviorTreeExampleRuntime.hpp>
#include <humanoid/bt/ActionNode.h>
#include <humanoid/bt/CompositeNode.h>
#include <humanoid/bt/MissionNode.h>
#include <humanoid/bt/ParallelNode.h>
#include <humanoid/bt/SequenceNode.h>
#include <humanoid/core/CommandType.h>

namespace {

int RunFlagCeremony() {
  humanoid::examples::BehaviorTreeExampleRuntime runtime;

  humanoid::bt::BTChildren ceremony_actions;
  ceremony_actions.push_back(
      std::make_unique<humanoid::bt::ActionNode>("RaiseFlag", [&runtime](humanoid::bt::BTContext&) {
        runtime.Print("flag ceremony: flag raised");
        return humanoid::bt::BTStatus::Success;
      }));
  ceremony_actions.push_back(std::make_unique<humanoid::bt::ActionNode>(
      "PlayAnthem", [&runtime](humanoid::bt::BTContext&) {
        runtime.Print("flag ceremony: anthem cue completed");
        return humanoid::bt::BTStatus::Success;
      }));

  humanoid::mission::Mission preparation = humanoid::examples::MakeMission(
      201U, 1U, "CeremonyPreparation",
      humanoid::examples::MakeCommand(201U, humanoid::core::CommandType::Stand));

  humanoid::bt::BTChildren sequence;
  sequence.push_back(std::make_unique<humanoid::bt::MissionNode>(
      "PrepareCeremony", runtime.MissionExecutor(), std::move(preparation)));
  sequence.push_back(std::make_unique<humanoid::bt::ParallelNode>("CeremonyParallel", 0U, 1U,
                                                                  std::move(ceremony_actions)));

  auto root = std::make_unique<humanoid::bt::SequenceNode>(
      "FlagCeremonySequence", humanoid::bt::SequenceMemoryPolicy::Memory, std::move(sequence));
  return runtime.Run("Flag Ceremony", 82001U, std::move(root)) ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace

int main() {
  try {
    return RunFlagCeremony();
  } catch (const std::exception& exception) {
    std::cerr << "Flag ceremony example failed: " << exception.what() << '\n';
  } catch (...) {
    std::cerr << "Flag ceremony example failed with an unknown error\n";
  }
  return EXIT_FAILURE;
}
