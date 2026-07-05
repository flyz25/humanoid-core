# Cancellation Framework

Milestone 7.4 introduces framework-wide cooperative cancellation primitives for
runtime, mission, command, and future behavior-tree execution paths.

## Public Header

```cpp
#include <humanoid/runtime/Cancellation.h>
```

`CancellationSource`, `CancellationToken`, `CancellationRegistration`, and
`CancellationCallback` are in `humanoid::runtime`. The header is also available
through `<humanoid/core.hpp>`.

## Ownership Model

`CancellationSource` owns cancellation authority. Calling `Cancel()` requests
cooperative cancellation exactly once. Later calls return `false` and do not
invoke callbacks again.

`CancellationToken` is a copyable observation handle. It can be passed to
mission steps, command execution, runtime services, or future behavior-tree
nodes without granting authority to cancel the parent operation.

`CancellationRegistration` is a move-only RAII object returned by `Register()`.
Destroying or explicitly unregistering it removes the callback if cancellation
has not started.

## Linked Cancellation

A source may be constructed from a parent token:

```cpp
humanoid::runtime::CancellationSource parent;
humanoid::runtime::CancellationSource child{parent.Token()};
```

Cancelling the parent cancels the child. Cancelling the child does not cancel
the parent. This supports nested execution such as mission -> step -> command
without creating dependencies between those layers.

## Callbacks

Callbacks are invoked outside internal locks. Exceptions thrown by callbacks
are contained so one callback cannot prevent later callbacks from running and
cannot escape `Cancel()`.

If `Register()` is called after cancellation has already been requested, the
callback is invoked before `Register()` returns and the returned registration is
empty.

Destroying a registration prevents future callback invocation when cancellation
has not started. If cancellation is already in progress, the callback may have
already been selected for invocation.

## ExecutionContext Bridge

`ExecutionContext::RuntimeCancellationToken()` exposes the framework-wide token
for runtime integrations while the existing `std::stop_token` API remains
available for compatibility. `ExecutionContext::RequestCancellation()` requests
both cancellation mechanisms.

## Dependency Boundary

```text
Mission / Command / Runtime / Future behavior trees
  -> humanoid::runtime::CancellationToken
  -> humanoid::runtime::CancellationSource
  -> C++ standard library
```

The cancellation framework contains no SDK, robot adapter, mission execution,
command dispatch, behavior-tree, ROS2, or vendor-specific logic.
