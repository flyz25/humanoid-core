/**
 * @file resource_manager_unit_test.cpp
 * @brief Validates runtime resource ownership, timeout, and concurrency rules.
 */

#include <atomic>
#include <barrier>
#include <chrono>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

#include <humanoid/runtime/ResourceManager.h>

namespace {

using humanoid::runtime::ResourceLock;
using humanoid::runtime::ResourceLockMode;
using humanoid::runtime::ResourceManager;

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

void TestExclusiveOwnershipBlocksOtherOwners() {
  ResourceManager manager;
  ResourceLock exclusive = manager.Acquire("robot:g1", ResourceLockMode::Exclusive);
  Check(exclusive.OwnsLock(), "Exclusive resource was not acquired");
  Check(exclusive.Handle().IsValid(), "Exclusive handle is invalid");
  Check(exclusive.Handle().ResourceId() == "robot:g1", "Exclusive handle resource mismatch");
  Check(exclusive.Handle().Mode() == ResourceLockMode::Exclusive, "Exclusive handle mode mismatch");
  Check(manager.ActiveLockCount("robot:g1") == 1U, "Exclusive active count mismatch");

  Check(!manager.TryAcquire("robot:g1", ResourceLockMode::Exclusive).has_value(),
        "Second exclusive owner acquired the same resource");
  Check(!manager.TryAcquire("robot:g1", ResourceLockMode::Shared).has_value(),
        "Shared owner acquired an exclusively owned resource");

  Check(exclusive.Release(), "Explicit exclusive release failed");
  Check(!exclusive.OwnsLock(), "Released exclusive lock still reports ownership");
  Check(manager.ActiveLockCount("robot:g1") == 0U, "Exclusive release did not clear count");

  std::optional<ResourceLock> reacquired =
      manager.TryAcquire("robot:g1", ResourceLockMode::Exclusive);
  Check(reacquired.has_value() && reacquired->OwnsLock(), "Exclusive reacquire failed");
}

void TestSharedOwnersBlockExclusiveOwner() {
  ResourceManager manager;
  ResourceLock first = manager.Acquire("robot:g1", ResourceLockMode::Shared);
  ResourceLock second = manager.Acquire("robot:g1", ResourceLockMode::Shared);

  Check(first.OwnsLock(), "First shared resource was not acquired");
  Check(second.OwnsLock(), "Second shared resource was not acquired");
  Check(manager.ActiveLockCount("robot:g1") == 2U, "Shared active count mismatch");
  Check(!manager.TryAcquire("robot:g1", ResourceLockMode::Exclusive).has_value(),
        "Exclusive owner acquired while shared owners are active");

  Check(manager.Release(first.Handle()), "Manager release through copied handle failed");
  Check(manager.ActiveLockCount("robot:g1") == 1U, "Copied handle release count mismatch");
  Check(!first.Release(), "RAII lock released a lease already released by handle");
  Check(second.Release(), "Second shared release failed");

  std::optional<ResourceLock> exclusive =
      manager.TryAcquire("robot:g1", ResourceLockMode::Exclusive);
  Check(exclusive.has_value() && exclusive->OwnsLock(),
        "Exclusive owner was not acquired after shared release");
}

void TestTimeoutHandling() {
  ResourceManager manager;
  ResourceLock shared = manager.Acquire("robot:g1", ResourceLockMode::Shared);

  const auto start = std::chrono::steady_clock::now();
  std::optional<ResourceLock> exclusive =
      manager.Acquire("robot:g1", ResourceLockMode::Exclusive, std::chrono::milliseconds{30});
  const auto elapsed = std::chrono::steady_clock::now() - start;

  Check(!exclusive.has_value(), "Timed exclusive acquisition succeeded while shared owner existed");
  Check(elapsed >= std::chrono::milliseconds{20}, "Timed acquisition returned too early");
  Check(manager.ActiveLockCount("robot:g1") == 1U, "Timed acquisition changed active count");

  Check(shared.Release(), "Shared release after timeout failed");
  exclusive =
      manager.Acquire("robot:g1", ResourceLockMode::Exclusive, std::chrono::milliseconds{30});
  Check(exclusive.has_value() && exclusive->OwnsLock(),
        "Timed exclusive acquisition failed after resource was released");
}

void TestWaitingSharedOwnerAcquiresAfterExclusiveRelease() {
  ResourceManager manager;
  ResourceLock exclusive = manager.Acquire("robot:g1", ResourceLockMode::Exclusive);
  std::atomic<bool> acquired{false};
  std::atomic<bool> failed{false};

  {
    std::jthread waiter{[&manager, &acquired, &failed]() {
      std::optional<ResourceLock> shared =
          manager.Acquire("robot:g1", ResourceLockMode::Shared, std::chrono::milliseconds{500});
      if (!shared.has_value() || !shared->OwnsLock()) {
        failed = true;
        return;
      }
      acquired = true;
    }};

    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    Check(!acquired.load(), "Shared owner acquired before exclusive release");
    Check(exclusive.Release(), "Exclusive release for waiting shared owner failed");
  }

  Check(!failed.load(), "Waiting shared owner failed to acquire after exclusive release");
  Check(acquired.load(), "Waiting shared owner did not acquire after exclusive release");
  Check(manager.ActiveLockCount("robot:g1") == 0U, "Waiting shared owner leaked a lock");
}

void TestInvalidResourceIds() {
  ResourceManager manager;
  ResourceLock invalid = manager.Acquire("", ResourceLockMode::Exclusive);
  Check(!invalid.OwnsLock(), "Empty resource id produced an owned lock");
  Check(!manager.TryAcquire("", ResourceLockMode::Shared).has_value(),
        "TryAcquire accepted an empty resource id");
  Check(!manager.Acquire("", ResourceLockMode::Shared, std::chrono::milliseconds{1}).has_value(),
        "Timed acquire accepted an empty resource id");
  Check(manager.ActiveLockCount("") == 0U, "Empty resource id has active locks");
}

void TestConcurrentRuntimeInstances() {
  constexpr int kWorkerCount = 8;
  constexpr int kIterations = 80;
  ResourceManager manager;
  std::barrier<> start_barrier{kWorkerCount};
  std::atomic<int> active_exclusive{0};
  std::atomic<int> max_active_exclusive{0};
  std::atomic<int> failed_acquisitions{0};
  std::vector<std::jthread> workers;
  workers.reserve(kWorkerCount);

  for (int worker = 0; worker < kWorkerCount; ++worker) {
    workers.emplace_back([&manager, &start_barrier, &active_exclusive, &max_active_exclusive,
                          &failed_acquisitions]() {
      start_barrier.arrive_and_wait();
      for (int iteration = 0; iteration < kIterations; ++iteration) {
        std::optional<ResourceLock> lock = manager.Acquire("robot:g1", ResourceLockMode::Exclusive,
                                                           std::chrono::milliseconds{500});
        if (!lock.has_value() || !lock->OwnsLock()) {
          ++failed_acquisitions;
          continue;
        }

        const int active = ++active_exclusive;
        int observed = max_active_exclusive.load();
        while (active > observed && !max_active_exclusive.compare_exchange_weak(observed, active)) {
        }
        std::this_thread::sleep_for(std::chrono::microseconds{50});
        --active_exclusive;
      }
    });
  }

  workers.clear();
  Check(failed_acquisitions.load() == 0, "Concurrent exclusive acquisition timed out");
  Check(max_active_exclusive.load() == 1, "Multiple exclusive owners were active concurrently");
  Check(manager.ActiveLockCount("robot:g1") == 0U, "Concurrent test leaked resource locks");
}

} // namespace

int main() {
  try {
    TestExclusiveOwnershipBlocksOtherOwners();
    TestSharedOwnersBlockExclusiveOwner();
    TestTimeoutHandling();
    TestWaitingSharedOwnerAcquiresAfterExclusiveRelease();
    TestInvalidResourceIds();
    TestConcurrentRuntimeInstances();
  } catch (...) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
