/**
 * @file blackboard_unit_test.cpp
 * @brief Validates typed, namespaced, concurrent runtime blackboard access.
 */

#include <atomic>
#include <barrier>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <humanoid/runtime/Blackboard.h>

namespace {

struct Payload final {
  int sequence{0};
  std::string label;
};

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

void TestTypedStoreAndGet() {
  humanoid::runtime::Blackboard blackboard;

  Check(blackboard.Store("runtime", "attempt", 7), "Integer value was not stored");
  Check(blackboard.Store("runtime", "name", std::string{"inspection"}),
        "String value was not stored");
  Check(blackboard.Contains("runtime", "attempt"), "Stored key is not present");

  const std::shared_ptr<const int> attempt = blackboard.Get<int>("runtime", "attempt");
  const std::shared_ptr<const std::string> name = blackboard.Get<std::string>("runtime", "name");
  Check(attempt && *attempt == 7, "Integer value mismatch");
  Check(name && *name == "inspection", "String value mismatch");
  Check(!blackboard.Get<double>("runtime", "attempt"), "Type mismatch returned a value");
  Check(!blackboard.Store("", "key", 1), "Empty namespace was accepted");
  Check(!blackboard.Store("runtime", "", 1), "Empty key was accepted");
}

void TestNamespaceIsolationAndReplacement() {
  humanoid::runtime::Blackboard blackboard;
  Check(blackboard.Store("mission-a", "status", std::string{"running"}),
        "First namespace value was not stored");
  Check(blackboard.Store("mission-b", "status", std::string{"paused"}),
        "Second namespace value was not stored");

  const std::shared_ptr<const std::string> first =
      blackboard.Get<std::string>("mission-a", "status");
  const std::shared_ptr<const std::string> second =
      blackboard.Get<std::string>("mission-b", "status");
  Check(first && *first == "running", "First namespace value mismatch");
  Check(second && *second == "paused", "Second namespace value mismatch");

  Check(blackboard.Store("mission-a", "status", std::string{"completed"}),
        "Replacement value was not stored");
  Check(*first == "running", "Replacement invalidated an existing shared handle");
  Check(*blackboard.Get<std::string>("mission-a", "status") == "completed",
        "Replacement value mismatch");
}

void TestSharedOwnershipAndRemoval() {
  humanoid::runtime::Blackboard blackboard;
  auto mutable_payload = std::make_shared<Payload>(Payload{42, "retained"});
  Check(blackboard.Store("shared", "payload", mutable_payload), "Shared payload was not stored");

  const std::shared_ptr<const Payload> retained = blackboard.Get<Payload>("shared", "payload");
  mutable_payload.reset();
  Check(retained && retained->sequence == 42, "Blackboard did not retain shared payload");
  Check(blackboard.Remove("shared", "payload"), "Shared payload was not removed");
  Check(!blackboard.Contains("shared", "payload"), "Removed payload remains present");
  Check(retained->label == "retained", "Removal invalidated an existing shared handle");
  Check(!blackboard.Remove("shared", "payload"), "Missing payload was removed");

  std::shared_ptr<Payload> null_payload;
  Check(!blackboard.Store("shared", "null", null_payload), "Null shared payload was accepted");
}

void TestClear() {
  humanoid::runtime::Blackboard blackboard;
  Check(blackboard.Store("first", "a", 1), "First clear value was not stored");
  Check(blackboard.Store("first", "b", 2), "Second clear value was not stored");
  Check(blackboard.Store("second", "a", 3), "Third clear value was not stored");

  Check(blackboard.Clear("first") == 2U, "Namespace clear count mismatch");
  Check(!blackboard.Contains("first", "a"), "Namespace clear retained a value");
  Check(blackboard.Contains("second", "a"), "Namespace clear affected another namespace");
  Check(blackboard.Clear() == 1U, "Global clear count mismatch");
  Check(blackboard.Clear() == 0U, "Empty global clear count mismatch");
}

void TestConcurrentStress() {
  constexpr int kWriterCount = 4;
  constexpr int kReaderCount = 4;
  constexpr int kIterations = 2000;
  auto blackboard = std::make_shared<humanoid::runtime::Blackboard>();
  std::barrier<> start_barrier{kWriterCount + kReaderCount};
  std::atomic<int> invalid_reads{0};
  std::vector<std::jthread> workers;
  workers.reserve(kWriterCount + kReaderCount);

  for (int writer = 0; writer < kWriterCount; ++writer) {
    workers.emplace_back([blackboard, &start_barrier, writer]() {
      start_barrier.arrive_and_wait();
      for (int iteration = 0; iteration < kIterations; ++iteration) {
        static_cast<void>(blackboard->Store("stress", "shared", iteration));
        static_cast<void>(
            blackboard->Store("workers", std::to_string(writer), Payload{iteration, "active"}));
        if (iteration % 17 == 0) {
          static_cast<void>(blackboard->Remove("stress", "shared"));
        }
        if (writer == 0 && iteration % 101 == 0) {
          static_cast<void>(blackboard->Clear("workers"));
        }
      }
    });
  }

  for (int reader = 0; reader < kReaderCount; ++reader) {
    workers.emplace_back([blackboard, &start_barrier, &invalid_reads]() {
      start_barrier.arrive_and_wait();
      for (int iteration = 0; iteration < kIterations; ++iteration) {
        const std::shared_ptr<const int> value = blackboard->Get<int>("stress", "shared");
        if (value && (*value < 0 || *value >= kIterations)) {
          ++invalid_reads;
        }
        static_cast<void>(blackboard->Contains("workers", "0"));
      }
    });
  }

  workers.clear();
  Check(invalid_reads.load() == 0, "Concurrent read observed an invalid value");
  Check(blackboard->Store("stress", "final", 9001), "Final stress value was not stored");
  const std::shared_ptr<const int> final_value = blackboard->Get<int>("stress", "final");
  Check(final_value && *final_value == 9001, "Final stress value mismatch");
}

} // namespace

int main() {
  try {
    TestTypedStoreAndGet();
    TestNamespaceIsolationAndReplacement();
    TestSharedOwnershipAndRemoval();
    TestClear();
    TestConcurrentStress();
  } catch (...) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
