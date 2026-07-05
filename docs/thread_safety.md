# Thread-Safety Guarantees

This document defines the thread-safety expectations for the current
humanoid-core skeleton.

## Summary

| Component | Guarantee |
| --- | --- |
| `RobotFactoryRegistry` | Thread-safe registration, lookup, factory count, and vendor listing. |
| `PluginRegistry` | Thread-safe plugin registration, unregistration, lifecycle updates, metadata lookup, and registry snapshots. |
| `PluginFactory` | Thread-safe plugin creator registration, unregistration, creation, destruction, enumeration, and active-instance accounting. |
| `UnitreeG1Plugin` | Thread-safe lifecycle state transitions through an internal mutex. |
| `UnitreeG1Adapter` plugin skeleton | Thread-safe skeleton lifecycle and connection reads through atomics. |
| `LoggerManager` | Thread-safe sink registration, sink clearing, severity updates, and logging calls. |
| `RobotStateManager` | Thread-safe state updates, resets, snapshots, and scalar field reads. |
| `ExecutionContext` | Thread-safe state, mission association, timestamp, current-step, metadata, cancellation, and stored-state snapshot operations. Execution ID and scope are immutable. |
| `CancellationSource`, `CancellationToken`, `CancellationRegistration` | Thread-safe cooperative cancellation request, observation, callback registration, callback unregistration, and linked-source propagation. Callbacks run outside internal locks. |
| `Blackboard` | Thread-safe namespaced store, exact-type lookup, replacement, removal, and clear operations. Returned immutable values have shared lifetime independent of map locks. |
| `ResourceManager` | Thread-safe shared and exclusive logical resource acquisition, timed acquisition, non-blocking acquisition, handle release, active-count queries, and RAII release through `ResourceLock`. |
| `RuntimeScheduler` | Thread-safe runtime job submission, priority queueing, parallel and sequential dispatch, cooperative pause/resume, cooperative stop, lifecycle snapshots, statistics, and idempotent shutdown. Job callbacks run outside scheduler locks. |
| `BTContext` | Thread-safe replacement and retrieval of shared runtime execution context and blackboard dependencies. |
| `BehaviorTree` | Thread-safe serialized lifecycle, tick, reset, shutdown, status, and root queries. Node callbacks execute under tree serialization. |
| `BehaviorTreeFactory` | Thread-safe node creator registration, unregistration, lookup, enumeration, node creation, and tree creation. Creator callbacks run outside factory locks. |
| `CompositeNode`, `SequenceNode`, `SelectorNode`, `ParallelNode` | Thread-safe child ownership, lifecycle propagation, traversal state, and tick operations. Parallel node ticks different child nodes concurrently while serializing access to its own child collection. |
| `DecoratorNode`, `InverterNode`, `RepeatNode`, `RetryNode`, `SucceederNode`, `FailerNode`, `LimiterNode`, `TimeoutNode` | Thread-safe child ownership, lifecycle propagation, local policy state, and tick operations for one owned child node. |
| `ActionNode`, `ConditionNode`, `WaitNode`, `DelayNode`, `CommandNode`, `MissionNode` | Thread-safe leaf lifecycle, local state, and tick operations. Command and mission nodes delegate concurrency to injected framework services. |
| `SafetyValidator` | Immutable after construction; safe to share across threads when callers provide independent validation contexts. |
| `CommandExecutionPipeline` | Thread-safe submission, queued cancellation, callback subscription, metrics, history snapshots, and idempotent shutdown. Executor and lifecycle callbacks run outside pipeline locks. |
| `CommandQueue` | Thread-safe bounded submission, priority dequeue, cancellation, statistics, and idempotent shutdown across concurrent producers and consumers. |
| `CommandDispatcher` | Thread-safe synchronous execution, asynchronous queueing, queued-command cancellation, and idempotent shutdown. Adapter calls are serialized. |
| `ConditionEvaluator` | Thread-safe capability context updates and condition reads. Robot state is copied from `RobotStateManager` before evaluation. |
| `MissionExecutor` | Thread-safe lifecycle requests and state snapshots. One owned worker executes mission steps serially; callbacks into `CommandDispatcher` occur without holding executor state locks. |
| `TelemetryService` | Thread-safe start, stop, subscribe, and unsubscribe operations. Listener callbacks are invoked outside service locks. |
| `SdkWrapper` | Thread-safe public methods through internal serialization of Unitree SDK2 access. The read-only heartbeat worker shares the same mutex and invokes state callbacks outside the SDK lock. |
| `LocoAdapter`, `HandAdapter`, `AudioAdapter` | Thread-safe public methods through per-adapter mutexes around owned Unitree SDK2 clients. |
| `LocoClientWrapper` | Thread-safe public methods through internal serialization of SDK access. State-manager callback installation delegates to `SdkWrapper`. |
| `UnitreeG1Adapter` | Thread-safe public methods through adapter-level serialization. |
| `UnitreeRobotFactory` | Immutable after construction; safe to share between threads. |
| Managers in `robot`, `motion`, `gesture`, `safety`, `diagnostics`, and `configuration` | Not internally synchronized; require external synchronization if mutated or queried concurrently. |
| Value types and enums | Safe to copy and read concurrently after construction. |

## Managers

The Milestone 1 managers are lightweight dependency holders. They are intended
for composition-time wiring and single-threaded ownership unless a downstream
application provides external synchronization.

`CoreContext` follows the same composition-time rule for replacing injected
dependencies. The injected `RobotStateManager` remains thread-safe after it is
published through the context.

Examples that require external synchronization:

- Concurrent `setController()` and `startGesture()` on `GestureManager`.
- Concurrent `setConfiguration()` and `value()` on `ConfigManager`.
- Concurrent `setRobot()` and `activate()` on `RobotManager`.

`RobotStateManager` is an exception to this composition-time-manager rule. It
is a runtime latest-state cache and protects its state with `std::shared_mutex`.
Writers use `std::unique_lock`; readers use `std::shared_lock`.

## Services

`CommandExecutionPipeline` protects queue state, execution records, metrics,
history, callback subscriptions, and lifecycle state with one mutex. Workers
select queued work under the mutex and invoke the injected executor after
releasing it. Lifecycle callbacks are copied under lock and invoked after lock
release. Running executor callbacks are not interrupted by cancellation or
shutdown; cancellation applies only to queued executions.

`CommandQueue` protects queue, outstanding-command indexes, statistics, and
lifecycle state with one mutex. Workers select and remove work while holding the
mutex, then invoke the injected executor after releasing it. Workers sleep on a
condition variable when no work is available. Multiple workers may execute
callbacks concurrently.

`SafetyValidator` stores only value-type policy options. It does not cache robot
state, own adapter handles, or perform I/O. `CommandDispatcher` provides a fresh
validation context for each command from either an injected `RobotStateManager`
or the legacy adapter state query.

`CommandDispatcher` protects lifecycle and synchronous command IDs separately
and serializes all calls to its injected `IRobotAdapter` with an adapter mutex.
Safety validation and adapter forwarding occur under that serialized adapter
section for legacy state queries. Its asynchronous path delegates worker
lifecycle to `CommandQueue`. `Cancel()` removes only queued work; running calls
require the adapter to return.
`Shutdown()` rejects new work, cancels queued work, and waits for synchronous
and asynchronous calls already in progress.

`ConditionEvaluator` reads state only by copying a `RobotState` snapshot from
the injected `RobotStateManager`. Capability context updates are protected by an
internal mutex. The evaluator does not call adapters, plugins, SDK wrappers, or
vendor SDKs.

`TelemetryService` owns a `std::jthread` while running and sleeps on a condition
variable between state samples. `Stop()` requests cooperative cancellation and
wakes the worker, so shutdown is not tied to the full polling interval.

Subscriber storage is protected separately from lifecycle state. The service
copies the current listener callbacks under a shared lock and invokes callbacks
after releasing internal locks. A listener may receive one final copied state
after `Unsubscribe()` if publication was already in progress.

## Runtime Cancellation

`CancellationSource` owns a shared cancellation state protected by a mutex.
`Cancel()` is monotonic: only the first caller transitions the state and drains
registered callbacks. Callback invocation occurs after the lock is released.
Callback exceptions are contained so cancellation does not fail partway through
notification.

`CancellationToken` is copyable and can be read or used for callback
registration concurrently. `CancellationRegistration` is move-only and
unregisters its pending callback on destruction. Registration destruction
prevents future invocation if cancellation has not started; if cancellation is
already in progress, the callback may have already been selected and can still
run.

Linked cancellation sources register with their parent token. Parent
cancellation propagates to children; child cancellation does not propagate back
to the parent.

`ExecutionContext::RequestCancellation()` requests both the existing
`std::stop_token` path and the framework `CancellationToken` path. The existing
standard token API remains available for compatibility.

## Runtime Resources

`ResourceManager` protects its resource map with a mutex and coordinates
waiters through a condition variable. Shared locks may coexist when no exclusive
owner or waiting exclusive owner exists. Exclusive locks require no active
shared or exclusive owner. Waiting exclusive owners are given preference over
new shared owners to prevent starvation.

`ResourceLock` is move-only and releases its lease on destruction. A copied
`ResourceHandle` may also release an active lease through `ResourceManager`;
after that, a still-live RAII object will observe that the lease has already
been released when its own `Release()` runs. Timed acquisition uses a steady
clock and returns empty on timeout. The manager coordinates one resource per
lease; callers that need multi-resource ownership must impose a higher-level
lock ordering policy.

## Runtime Scheduler

`RuntimeScheduler` protects job queue, lifecycle snapshots, worker ids, and
statistics with one mutex. Workers select schedulable jobs under that lock, then
release it before invoking the injected job callback.

Parallel jobs may run concurrently up to the configured worker count.
Sequential jobs require exclusive scheduler execution and start only when no
other job is running. Priority selection uses descending priority and FIFO
ordering inside the same priority.

Pause and stop are cooperative. `Pause()` marks the job and updates lifecycle
state; running callbacks must call `RuntimeJobContext::WaitIfPaused()` or poll
`IsPaused()` to suspend work. `Stop()` cancels queued jobs immediately and
requests cancellation for running jobs. Shutdown rejects new work, cancels
queued work, requests cancellation for running callbacks, and joins workers.

## Behavior Trees

`BTContext` protects dependency replacement with a shared mutex. Getters return
copied `std::shared_ptr` handles, so node code never observes references to
context internals.

`BehaviorTree` serializes `Initialize()`, `Tick()`, `Reset()`, `Shutdown()`,
status reads, and root reads with one mutex. Root node callbacks run under this
serialization, so a single tree never ticks the same root concurrently. A node
implementation remains responsible for synchronizing any shared state it owns
outside the tree.

`BehaviorTreeFactory` protects registered node creators with a shared mutex.
Creator callbacks are copied under lock and invoked after lock release.

`CompositeNode` protects child ownership and lifecycle propagation with one
mutex. `SequenceNode` and `SelectorNode` protect memory traversal indexes with
the same lock. `ParallelNode` serializes access to its child collection while
dispatching one asynchronous tick per child; each child node is responsible for
the thread-safety of its own internals.

`DecoratorNode` protects child ownership and lifecycle propagation with one
mutex. Concrete decorators use the same lock for local policy state such as
repeat counts, retry attempts, tick limits, and timeout start times. Decorators
tick only their owned child and do not share mutable child ownership with other
nodes.

Leaf nodes protect callback handles, local timers, command futures, mission
start state, and terminal status with per-node mutexes. `CommandNode` submits
work through `CommandDispatcher::ExecuteAsync()` and polls without blocking.
`MissionNode` starts work through `MissionExecutor` and polls executor status.
Injected callbacks in `ActionNode`, `ConditionNode`, and `WaitNode` must provide
their own synchronization for state they capture outside the node.

## Registry

`RobotFactoryRegistry` protects its factory collection with a mutex. Returned
factory instances are held by `std::shared_ptr`; factory implementations must
preserve their own thread-safety guarantees.

`PluginRegistry` protects metadata and lifecycle state with `std::shared_mutex`.
Registration, unregistration, and lifecycle updates use exclusive access.
Metadata lookup, lifecycle lookup, registry snapshots, and count reads use
shared access. The registry does not own plugin implementation objects.

`PluginFactory` protects creator storage and active-instance accounting with
`std::shared_mutex`. Creator registration, unregistration, and instance-count
updates use exclusive access. Enumeration is delegated to the injected
`PluginRegistry`. Plugin creator callbacks and plugin `Shutdown()` calls are not
invoked while the factory lock is held. Instance accounting includes plugin
creation that is already in progress, so unregistration is rejected until
in-flight creation or destruction has completed.

`UnitreeG1Plugin` protects its skeleton lifecycle flags with a mutex.
`UnitreeG1Adapter` uses atomics for initialized and connected state because the
skeleton contains no SDK client, transport handle, or mutable robot state cache.

## Logging

`LoggerManager` protects sink collection and minimum severity state. Logging
copies the current sink list before calling sinks so sink callbacks do not run
under the manager lock. Individual `LogSink` implementations are responsible
for their own thread-safety.

## SDK Wrapper and Adapter

`SdkWrapper` serializes high-level SDK boundary calls and delegates direct
Unitree SDK2 command execution to `LocoAdapter`, `HandAdapter`, and
`AudioAdapter`. Each command adapter protects its owned SDK client with a mutex.
The communication worker uses a condition variable and SDK timeout settings
rather than busy waiting. `LocoClientWrapper` is a compatibility facade over
`SdkWrapper`. `UnitreeG1Adapter` serializes adapter state changes and wrapper
access. This prevents concurrent command interleaving inside one adapter
instance.

The Unitree SDK may own process-level transport state internally. Applications
should avoid creating multiple active Unitree SDK wrapper instances for the same
robot interface unless the SDK vendor documents that use case as safe.
