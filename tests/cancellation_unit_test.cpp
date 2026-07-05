/**
 * @file cancellation_unit_test.cpp
 * @brief Validates framework-wide cooperative cancellation primitives.
 */

#include <atomic>
#include <barrier>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <vector>

#include <humanoid/runtime/Cancellation.h>
#include <humanoid/runtime/ExecutionContext.h>

namespace {

using humanoid::runtime::CancellationRegistration;
using humanoid::runtime::CancellationSource;
using humanoid::runtime::ExecutionContext;

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

void TestBasicCancellationAndImmediateCallback() {
  CancellationSource source;
  const auto token = source.Token();
  std::atomic<int> callback_count{0};

  CancellationRegistration first = token.Register([&callback_count]() { ++callback_count; });
  CancellationRegistration second = source.Register([&callback_count]() { ++callback_count; });
  Check(token.IsCancellationPossible(), "Token does not report cancellation state");
  Check(first.IsRegistered(), "First callback was not registered");
  Check(second.IsRegistered(), "Second callback was not registered");
  Check(!token.IsCancelled(), "Fresh token reports cancelled");

  Check(source.Cancel(), "First cancel request was not accepted");
  Check(token.IsCancelled(), "Token did not observe cancellation");
  Check(source.IsCancelled(), "Source did not report cancellation");
  Check(callback_count.load() == 2, "Registered callbacks were not invoked");
  Check(!first.IsRegistered(), "First registration still reports pending after cancel");
  Check(!second.IsRegistered(), "Second registration still reports pending after cancel");
  Check(!source.Cancel(), "Second cancel request was accepted");

  CancellationRegistration immediate = token.Register([&callback_count]() { ++callback_count; });
  Check(!immediate.IsRegistered(), "Post-cancel registration reports pending");
  Check(callback_count.load() == 3, "Post-cancel callback was not invoked immediately");
}

void TestUnregisterPreventsCallback() {
  CancellationSource source;
  std::atomic<int> callback_count{0};
  CancellationRegistration registration =
      source.Register([&callback_count]() { ++callback_count; });

  Check(registration.IsRegistered(), "Callback was not registered before unregister");
  Check(registration.Unregister(), "Unregister did not remove pending callback");
  Check(!registration.IsRegistered(), "Registration still reports pending after unregister");
  Check(source.Cancel(), "Cancel after unregister failed");
  Check(callback_count.load() == 0, "Unregistered callback was invoked");
}

void TestCallbackExceptionsAreContained() {
  CancellationSource source;
  std::atomic<int> callback_count{0};

  CancellationRegistration throwing = source.Register([]() { throw std::runtime_error{"cancel"}; });
  CancellationRegistration counted = source.Register([&callback_count]() { ++callback_count; });

  Check(throwing.IsRegistered(), "Throwing callback was not registered");
  Check(counted.IsRegistered(), "Counted callback was not registered");
  Check(source.Cancel(), "Cancel with throwing callback failed");
  Check(callback_count.load() == 1, "Callback after throwing callback did not run");
}

void TestConcurrentCancellation() {
  constexpr int kCallbackCount = 128;
  constexpr int kThreadCount = 12;
  CancellationSource source;
  std::atomic<int> callback_count{0};
  std::atomic<int> successful_cancel_count{0};
  std::vector<CancellationRegistration> registrations;
  registrations.reserve(kCallbackCount);

  for (int index = 0; index < kCallbackCount; ++index) {
    registrations.push_back(source.Register([&callback_count]() { ++callback_count; }));
  }

  std::barrier<> start_barrier{kThreadCount};
  std::vector<std::jthread> workers;
  workers.reserve(kThreadCount);
  for (int thread = 0; thread < kThreadCount; ++thread) {
    workers.emplace_back([&source, &start_barrier, &successful_cancel_count]() {
      start_barrier.arrive_and_wait();
      if (source.Cancel()) {
        ++successful_cancel_count;
      }
    });
  }

  workers.clear();
  Check(successful_cancel_count.load() == 1, "More than one concurrent cancel succeeded");
  Check(callback_count.load() == kCallbackCount, "Concurrent cancel missed callbacks");
  for (const CancellationRegistration& registration : registrations) {
    Check(!registration.IsRegistered(), "Registration remained pending after concurrent cancel");
  }
}

void TestNestedCancellation() {
  CancellationSource parent;
  CancellationSource child{parent.Token()};
  CancellationSource grandchild{child.Token()};
  std::atomic<int> child_callback_count{0};
  std::atomic<int> grandchild_callback_count{0};

  CancellationRegistration child_registration =
      child.Register([&child_callback_count]() { ++child_callback_count; });
  CancellationRegistration grandchild_registration =
      grandchild.Register([&grandchild_callback_count]() { ++grandchild_callback_count; });

  Check(child.Cancel(), "Child cancellation failed");
  Check(!parent.IsCancelled(), "Child cancellation propagated to parent");
  Check(grandchild.IsCancelled(), "Child cancellation did not propagate to grandchild");
  Check(child_callback_count.load() == 1, "Child callback did not run");
  Check(grandchild_callback_count.load() == 1, "Grandchild callback did not run");

  CancellationSource cancelled_parent;
  Check(cancelled_parent.Cancel(), "Parent pre-cancel failed");
  CancellationSource already_cancelled_child{cancelled_parent.Token()};
  Check(already_cancelled_child.IsCancelled(), "Child linked to cancelled parent is not cancelled");
}

void TestExecutionContextRuntimeCancellationToken() {
  ExecutionContext context;
  const auto runtime_token = context.RuntimeCancellationToken();
  std::atomic<int> callback_count{0};
  CancellationRegistration registration =
      runtime_token.Register([&callback_count]() { ++callback_count; });

  Check(runtime_token.IsCancellationPossible(),
        "ExecutionContext runtime token is not cancellable");
  Check(registration.IsRegistered(), "ExecutionContext runtime callback was not registered");
  Check(context.RequestCancellation(), "ExecutionContext cancellation did not succeed");
  Check(context.CancellationRequested(), "ExecutionContext did not report cancellation");
  Check(context.CancellationToken().stop_requested(), "ExecutionContext std token was not stopped");
  Check(runtime_token.IsCancelled(), "ExecutionContext runtime token was not cancelled");
  Check(callback_count.load() == 1, "ExecutionContext runtime callback did not run");
}

} // namespace

int main() {
  try {
    TestBasicCancellationAndImmediateCallback();
    TestUnregisterPreventsCallback();
    TestCallbackExceptionsAreContained();
    TestConcurrentCancellation();
    TestNestedCancellation();
    TestExecutionContextRuntimeCancellationToken();
  } catch (...) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
