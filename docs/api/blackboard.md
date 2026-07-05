# Runtime Blackboard

Milestone 7.2 introduces a vendor-independent, thread-safe blackboard for
sharing typed runtime data across future execution engines.

## Public Header

```cpp
#include <humanoid/runtime/Blackboard.h>
```

`Blackboard`, `BlackboardNamespace`, and `BlackboardKey` are in
`humanoid::runtime`. The header is also available through
`<humanoid/core.hpp>`.

## Addressing

Every value is addressed by a non-empty namespace and non-empty key. The same
key may exist independently in multiple namespaces. Empty address components
are rejected and never create storage.

## Typed Values

`Store()` accepts either a value or an existing `std::shared_ptr<T>`. Values
passed by value are moved into blackboard-managed shared storage. Raw pointers
are rejected because their ownership and lifetime are ambiguous.

`Get<T>()` returns `std::shared_ptr<const T>` only when the namespace, key, and
exact requested type match. Missing entries and type mismatches return an empty
pointer. The API does not perform implicit numeric or inheritance conversion.

Stored values are immutable through blackboard handles. Applications that need
mutable shared state should store a separately synchronized domain object.
When a caller stores an existing `shared_ptr<T>` and retains a mutable alias,
that caller remains responsible for synchronizing mutations performed through
the original alias.

## Ownership

The blackboard and all returned handles share ownership. A handle remains valid
after another value replaces its address and after `Remove()` or `Clear()`
releases blackboard ownership. Removing an entry does not invalidate data still
owned by a caller.

## Operations

- `Store()` atomically adds or replaces one typed value.
- `Get<T>()` returns shared immutable ownership of an exact typed value.
- `Contains()` checks for a value of any type.
- `Remove()` removes one namespace/key entry.
- `Clear(namespace)` removes all values in one namespace.
- `Clear()` removes all values in all namespaces.

Clear operations return the number of entries removed.

## Thread Safety

All public methods may be called concurrently. Reads use shared locking and
mutations use exclusive locking. Locks protect the blackboard's maps and entry
replacement only; they are released before callers access returned values.
Immutability of returned `const T` prevents unsynchronized mutation through the
blackboard API.

## Dependency Boundary

```text
Future execution engines
  -> humanoid::runtime::Blackboard
    -> C++ standard library
```

The blackboard contains no vendor logic, mission logic, behavior-tree logic,
robot adapters, middleware, or SDK dependencies.
