/**
 * @file main.cpp
 * @brief Shows framework-wide cooperative cancellation and linked tokens.
 */

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <humanoid/runtime/Cancellation.h>

namespace {

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

} // namespace

int main() {
  try {
    humanoid::runtime::CancellationSource parent;
    humanoid::runtime::CancellationSource child{parent.Token()};
    std::atomic<int> callbacks{0};

    auto parent_registration = parent.Register([&callbacks]() { ++callbacks; });
    auto child_registration = child.Register([&callbacks]() { ++callbacks; });

    Check(parent_registration.IsRegistered(), "parent callback was not registered");
    Check(child_registration.IsRegistered(), "child callback was not registered");
    Check(parent.Cancel(), "parent cancellation did not transition state");
    Check(parent.IsCancelled(), "parent did not report cancellation");
    Check(child.IsCancelled(), "child did not observe linked cancellation");
    Check(callbacks.load() == 2, "linked cancellation callback count mismatch");
    Check(!parent.Cancel(), "second cancellation request transitioned state");

    std::cout << "Cancellation example callbacks=" << callbacks.load() << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
