/**
 * @file main.cpp
 * @brief Shows basic ExecutionContext lifecycle and cancellation usage.
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <humanoid/runtime/ExecutionContext.h>

namespace {

using humanoid::runtime::ExecutionContext;
using humanoid::runtime::ExecutionContextId;
using humanoid::runtime::ExecutionScope;
using humanoid::runtime::ExecutionState;

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

} // namespace

int main() {
  try {
    ExecutionContext context{ExecutionContextId{1001U}, ExecutionScope::Custom};
    context.SetState(ExecutionState::Starting);
    context.SetStartTimestamp(
        std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()));
    context.SetCurrentStep(1U);
    context.SetMetadataValue("example", "execution_context");
    context.SetState(ExecutionState::Running);

    Check(context.Id() == 1001U, "execution id mismatch");
    Check(context.Scope() == ExecutionScope::Custom, "execution scope mismatch");
    Check(context.State() == ExecutionState::Running, "execution state mismatch");
    Check(context.MetadataValue("example").value_or("") == "execution_context",
          "metadata value mismatch");

    const auto runtime_token = context.RuntimeCancellationToken();
    Check(runtime_token.IsCancellationPossible(), "runtime token is not cancellable");
    Check(context.RequestCancellation(), "cancellation request failed");
    Check(runtime_token.IsCancelled(), "runtime token did not observe cancellation");
    Check(context.CancellationToken().stop_requested(),
          "std stop token did not observe cancellation");

    const auto snapshot = context.Snapshot();
    Check(snapshot.cancellationRequested, "snapshot did not include cancellation state");

    std::cout << "ExecutionContext example state=" << humanoid::runtime::toString(snapshot.state)
              << " cancelled=" << snapshot.cancellationRequested << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
