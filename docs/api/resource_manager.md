# Runtime Resource Manager

Milestone 7.3 introduces a vendor-independent runtime resource manager for
coordinating ownership of logical robot resources inside one process.

## Public Header

```cpp
#include <humanoid/runtime/ResourceManager.h>
```

`ResourceManager`, `ResourceLock`, `ResourceHandle`, and `ResourceLockMode` are
in `humanoid::runtime`. The header is also available through
`<humanoid/core.hpp>`.

## Resource Identity

Resources are addressed by non-empty string identifiers selected by the
application composition layer. Examples include a robot name, hardware slot, or
transport endpoint. Empty identifiers are rejected and never create internal
resource state.

## Lock Modes

`ResourceLockMode::Shared` allows multiple compatible runtime owners to hold a
resource concurrently. `ResourceLockMode::Exclusive` allows exactly one owner
and excludes both shared and exclusive owners.

The manager gives preference to waiting exclusive owners. Once an exclusive
waiter is queued, new shared acquisitions wait until the exclusive owner has
acquired and released the resource. This prevents writer starvation for
resources that need exclusive control.

## RAII Ownership

`Acquire()` returns a move-only `ResourceLock`. Destroying the lock releases its
lease automatically. `ResourceLock::Release()` may be called explicitly when
deterministic unlock timing is required.

`ResourceHandle` is an opaque value that identifies a manager-created lease. It
contains no SDK, adapter, or vendor data. `ResourceManager::Release(handle)` is
available for integration points that need to release through a copied handle;
normal code should prefer the RAII lock.

## Operations

- `Acquire(resource, mode)` blocks until a compatible lease is available.
- `TryAcquire(resource, mode)` attempts acquisition without blocking.
- `Acquire(resource, mode, timeout)` waits up to the requested duration and
  returns empty on timeout.
- `Release(handle)` releases an active lease by handle.
- `ActiveLockCount(resource)` reports current shared plus exclusive leases.

## Thread Safety

All public manager methods are thread-safe. Acquisition uses condition-variable
waiting and does not busy-wait. Timed acquisition uses `std::chrono` steady time.
All state changes wake waiting threads.

The resource manager supports single-resource leases. Code that needs to hold
multiple resources must define a higher-level lock ordering policy at the
application or execution-engine layer to avoid cross-resource deadlocks.

## Dependency Boundary

```text
Future execution engines
  -> humanoid::runtime::ResourceManager
    -> ResourceLock / ResourceHandle
      -> C++ standard library
```

The resource manager contains no SDK, robot adapter, mission, behavior-tree,
ROS2, or vendor-specific logic.
