/**
 * @file main.cpp
 * @brief Shows shared and exclusive runtime resource leases.
 */

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <humanoid/runtime/ResourceManager.h>

namespace {

using humanoid::runtime::ResourceLockMode;
using humanoid::runtime::ResourceManager;

void Check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error{message};
  }
}

} // namespace

int main() {
  try {
    ResourceManager manager;

    auto shared_a = manager.Acquire("robot:primary", ResourceLockMode::Shared);
    auto shared_b = manager.Acquire("robot:primary", ResourceLockMode::Shared);
    Check(shared_a.OwnsLock(), "first shared lock was not acquired");
    Check(shared_b.OwnsLock(), "second shared lock was not acquired");
    Check(manager.ActiveLockCount("robot:primary") == 2U, "shared lock count mismatch");
    Check(!manager.TryAcquire("robot:primary", ResourceLockMode::Exclusive).has_value(),
          "exclusive lock acquired while shared locks are active");

    Check(shared_a.Release(), "failed to release first shared lock");
    Check(shared_b.Release(), "failed to release second shared lock");
    auto exclusive = manager.TryAcquire("robot:primary", ResourceLockMode::Exclusive);
    Check(exclusive.has_value() && exclusive->OwnsLock(), "exclusive lock was not acquired");
    Check(manager.ActiveLockCount("robot:primary") == 1U, "exclusive lock count mismatch");

    std::cout << "ResourceManager example active_locks=" << manager.ActiveLockCount("robot:primary")
              << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
