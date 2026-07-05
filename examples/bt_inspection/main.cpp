/**
 * @file main.cpp
 * @brief Runs an inspection behavior tree through the shared runtime.
 */

#include <atomic>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>

#include <common/BehaviorTreeExampleRuntime.hpp>
#include <humanoid/bt/ActionNode.h>
#include <humanoid/bt/CompositeNode.h>
#include <humanoid/bt/ConditionNode.h>
#include <humanoid/bt/RetryNode.h>
#include <humanoid/bt/SelectorNode.h>

namespace {

int RunInspection() {
  humanoid::examples::BehaviorTreeExampleRuntime runtime;
  auto attempts = std::make_shared<std::atomic<int>>(0);

  auto cached_result = std::make_unique<humanoid::bt::ConditionNode>(
      "CachedInspectionResult", [&runtime](const humanoid::bt::BTContext&) {
        runtime.Print("inspection: no cached result available");
        return false;
      });

  auto inspection = std::make_unique<humanoid::bt::ActionNode>(
      "RunInspection", [&runtime, attempts](humanoid::bt::BTContext&) {
        const int attempt = ++(*attempts);
        runtime.Print("inspection: attempt " + std::to_string(attempt));
        return attempt < 3 ? humanoid::bt::BTStatus::Failure : humanoid::bt::BTStatus::Success;
      });

  humanoid::bt::BTChildren alternatives;
  alternatives.push_back(std::move(cached_result));
  alternatives.push_back(
      std::make_unique<humanoid::bt::RetryNode>("InspectionRetry", 3U, std::move(inspection)));

  auto root = std::make_unique<humanoid::bt::SelectorNode>(
      "InspectionSelector", humanoid::bt::SelectorMemoryPolicy::Memory, std::move(alternatives));
  return runtime.Run("Inspection", 83001U, std::move(root)) ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace

int main() {
  try {
    return RunInspection();
  } catch (const std::exception& exception) {
    std::cerr << "Inspection example failed: " << exception.what() << '\n';
  } catch (...) {
    std::cerr << "Inspection example failed with an unknown error\n";
  }
  return EXIT_FAILURE;
}
