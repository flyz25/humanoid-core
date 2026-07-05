/**
 * @file main.cpp
 * @brief Shows typed runtime blackboard storage and retrieval.
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <humanoid/runtime/Blackboard.h>

namespace {

struct RuntimeSample final {
  int sequence{0};
  std::string label;
};

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

} // namespace

int main() {
  try {
    humanoid::runtime::Blackboard blackboard;

    Check(blackboard.Store("runtime", "status", std::string{"running"}),
          "failed to store runtime status");
    Check(blackboard.Store("runtime", "sample", RuntimeSample{7, "blackboard"}),
          "failed to store runtime sample");

    const std::shared_ptr<const std::string> status =
        blackboard.Get<std::string>("runtime", "status");
    const std::shared_ptr<const RuntimeSample> sample =
        blackboard.Get<RuntimeSample>("runtime", "sample");

    Check(status && *status == "running", "status value mismatch");
    Check(sample && sample->sequence == 7 && sample->label == "blackboard",
          "sample value mismatch");
    Check(!blackboard.Get<int>("runtime", "status"), "type mismatch returned a value");

    Check(blackboard.Remove("runtime", "status"), "failed to remove runtime status");
    Check(!blackboard.Contains("runtime", "status"), "removed value is still present");
    Check(sample->label == "blackboard", "shared value was invalidated by removal");

    std::cout << "Blackboard example sample=" << sample->sequence << " label=" << sample->label
              << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
