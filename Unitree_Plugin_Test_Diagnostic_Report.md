# Unitree G1 Plugin Test Diagnostic Report

## 1. Test Location

CTest registers the failing test in `tests/CMakeLists.txt`.

- `tests/CMakeLists.txt:311-312` creates executable
  `humanoid_core_unitree_g1_plugin_skeleton_test`.
- `tests/CMakeLists.txt:314-316` links it to `humanoid::unitree_g1_plugin`.
- `tests/CMakeLists.txt:320-321` registers CTest name
  `humanoid_core_unitree_g1_plugin_skeleton_test`.

The test source is `tests/unitree_g1_plugin_skeleton_test.cpp`.

## 2. Source Files

Files in the call chain:

- `tests/CMakeLists.txt`
- `tests/unitree_g1_plugin_skeleton_test.cpp`
- `plugins/unitree/g1/include/humanoid/plugins/unitree/g1/UnitreeG1Adapter.hpp`
- `plugins/unitree/g1/src/UnitreeG1Adapter.cpp`
- `plugins/unitree/sdk/SdkWrapper.cpp`
- `plugins/unitree/sdk/LocoAdapter.cpp`
- `plugins/unitree/sdk/SdkClientSupport.h`
- `plugins/unitree/sdk/SdkTypes.h`
- `common/include/humanoid/common/Status.hpp`

## 3. Call Graph

Observed call chain for the failing assertion:

```text
CTest
  -> humanoid_core_unitree_g1_plugin_skeleton_test
    -> TestAdapterProductionContract()
      -> UnitreeG1Adapter adapter
      -> adapter.Connect() before Initialize()
      -> adapter.Initialize()
      -> adapter.Update() if Initialize() succeeded
      -> adapter.Connect()
        -> UnitreeG1Adapter::Impl::Connect()
          -> sdk_wrapper_.Connect()
            -> SdkWrapper::Connect()
              -> LocoAdapter::GetFsmId()
                -> Unitree SDK2 LocoClient::GetFsmId()
                  -> internal::InvokeSdkCommand()
                    -> internal::FromSdkReturn()
          -> ToStatus(SdkResult)
```

Relevant source references:

- `tests/unitree_g1_plugin_skeleton_test.cpp:104-167` defines
  `TestAdapterProductionContract()`.
- `plugins/unitree/g1/src/UnitreeG1Adapter.cpp:203-254` implements
  `UnitreeG1Adapter::Impl::Connect()`.
- `plugins/unitree/sdk/SdkWrapper.cpp:271-306` implements
  `SdkWrapper::Connect()`.
- `plugins/unitree/sdk/LocoAdapter.cpp:127-142` implements
  `LocoAdapter::GetFsmId()`.
- `plugins/unitree/sdk/SdkClientSupport.h:100-108` maps non-zero SDK integer
  return values to `SdkErrorCode::kUnknown`.
- `plugins/unitree/g1/src/UnitreeG1Adapter.cpp:68-89` maps SDK results to
  framework `Status`.

## 4. Expected Behavior

The failing check is in `tests/unitree_g1_plugin_skeleton_test.cpp:130-134`.

The test accepts only these statuses from `adapter.Connect()` after
`adapter.Initialize()`:

- `StatusCode::kOk`
- `StatusCode::kUnavailable`
- `StatusCode::kFailedPrecondition`

The accepted values are explicitly defined in:

```cpp
if (connect_status.code() != StatusCode::kOk &&
    connect_status.code() != StatusCode::kUnavailable &&
    connect_status.code() != StatusCode::kFailedPrecondition) {
  return Fail(kTestName, "connect failed with an unexpected status");
}
```

The framework status enum is defined in
`common/include/humanoid/common/Status.hpp:16-23`:

- `kOk`
- `kCancelled`
- `kInvalidArgument`
- `kUnavailable`
- `kFailedPrecondition`
- `kInternalError`

## 5. Actual Behavior

In the current local diagnostic run, the test passed.

The local probe against `build-repair-debug-on` returned:

```text
connect_before_initialize=kFailedPrecondition
initialize=kUnavailable
connect_after_initialize=kFailedPrecondition
```

The local reason was:

```text
route netlink socket is unavailable; Unitree SDK2 transport cannot be initialized in this environment
```

That path passes because `kFailedPrecondition` is accepted by the test.

For the reported failure message:

```text
Unitree G1 adapter production contract: connect failed with an unexpected status
```

the failing status must be outside the accepted set. In this call chain, the
only status produced by `UnitreeG1Adapter::Impl::Connect()` outside the accepted
set is `StatusCode::kInternalError`.

Evidence:

- `UnitreeG1Adapter::Impl::Connect()` returns `FailedPrecondition` before
  initialization at `plugins/unitree/g1/src/UnitreeG1Adapter.cpp:203-209`.
- If SDK is not built, it returns `Unavailable` at
  `plugins/unitree/g1/src/UnitreeG1Adapter.cpp:251-253`.
- SDK failures are converted by `ToStatus()` at
  `plugins/unitree/g1/src/UnitreeG1Adapter.cpp:68-89`.
- `ToStatus()` maps:
  - `kSuccess` -> `kOk`
  - `kSdkUnavailable`, `kConnectionFailed`, `kTimeout` -> `kUnavailable`
  - `kRobotFault` -> `kFailedPrecondition`
  - `kUnknown` -> `kInternalError`

Therefore the reported failing run is consistent with `SdkErrorCode::kUnknown`
being returned from the SDK abstraction and converted to
`StatusCode::kInternalError`.

## 6. Root Cause

The root cause is a mismatch between the test's offline contract and the
production SDK-backed implementation.

The test is registered as a normal CTest unit-style test, but it instantiates the
real `UnitreeG1Adapter` and links the real Unitree plugin. With
`ENABLE_UNITREE=ON`, the adapter calls the real SDK abstraction:

- `plugins/unitree/g1/CMakeLists.txt:21-24` links
  `humanoid::unitree_sdk_abstraction` and defines
  `HUMANOID_CORE_HAS_UNITREE_SDK=1`.
- `plugins/unitree/g1/src/UnitreeG1Adapter.cpp:212` calls
  `sdk_wrapper_.Connect()`.
- `plugins/unitree/sdk/SdkWrapper.cpp:284-285` calls
  `loco_adapter_.GetFsmId(fsm_id)`.
- `plugins/unitree/sdk/LocoAdapter.cpp:136-138` calls
  `impl_->client_->GetFsmId(sdk_fsm_id)`.

When the Unitree SDK call returns a non-zero integer without throwing,
`internal::FromSdkReturn()` maps it to `SdkErrorCode::kUnknown` at
`plugins/unitree/sdk/SdkClientSupport.h:100-108`.

`UnitreeG1Adapter::ToStatus()` maps `SdkErrorCode::kUnknown` to
`StatusCode::kInternalError` at
`plugins/unitree/g1/src/UnitreeG1Adapter.cpp:82-89`.

`StatusCode::kInternalError` is not accepted by the test at
`tests/unitree_g1_plugin_skeleton_test.cpp:130-134`, so the test fails with:

```text
connect failed with an unexpected status
```

This is not a pure logic-unit failure. It is an environment-sensitive SDK
integration path being exercised by a test that is still named and structured as
a skeleton/unit contract test.

## 7. Required Hardware

Required for the current test as written: **No, not strictly.**

The test is intended to pass offline because it explicitly accepts
`kUnavailable` and `kFailedPrecondition` from adapter initialization/connection.

Required for the successful `kOk` connection path: **Yes.**

The `kOk` path requires the SDK connection check to succeed through
`LocoClient::GetFsmId()`, which requires a working Unitree SDK2 transport and a
reachable robot/DDS endpoint.

## 8. SDK Dependency

SDK dependency: **Yes when `ENABLE_UNITREE=ON` and
`humanoid::unitree_sdk_abstraction` exists.**

Evidence:

- `plugins/unitree/g1/CMakeLists.txt:21-24` links the plugin to
  `humanoid::unitree_sdk_abstraction` and enables
  `HUMANOID_CORE_HAS_UNITREE_SDK=1`.
- `plugins/unitree/g1/src/UnitreeG1Adapter.cpp:15-20` includes SDK abstraction
  headers under that compile definition.
- `plugins/unitree/sdk/LocoAdapter.cpp:11-12` includes Unitree SDK2 headers.

SDK dependency when `ENABLE_UNITREE=OFF` or SDK abstraction is unavailable:
**No runtime SDK call is made.**

Evidence:

- `plugins/unitree/g1/src/UnitreeG1Adapter.cpp:183-185` returns
  `kUnavailable` from `Initialize()`.
- `plugins/unitree/g1/src/UnitreeG1Adapter.cpp:251-253` returns
  `kUnavailable` from `Connect()`.

## 9. Should Pass Offline?

Yes, by the test's own contract.

Evidence:

- `tests/unitree_g1_plugin_skeleton_test.cpp:117-120` accepts
  `Initialize()` returning `kUnavailable`.
- `tests/unitree_g1_plugin_skeleton_test.cpp:130-134` accepts
  `Connect()` returning `kUnavailable` or `kFailedPrecondition`.

However, the implementation can return `kInternalError` for a non-zero SDK
return code, which is not accepted by the test. That makes the offline result
environment-sensitive.

## 10. Recommended Classification

Recommended classification: **Integration Test**.

Reason:

- It constructs the production `UnitreeG1Adapter`.
- It links the production `humanoid::unitree_g1_plugin`.
- With SDK enabled, it executes the real SDK abstraction path.
- It does not use a mock SDK wrapper.
- It is sensitive to host networking/DDS/SDK runtime behavior.

It is not a pure unit test because a pure unit test would inject a fake SDK
wrapper or compile the adapter in SDK-disabled mode only.

It is not a hardware validation test in its current form because it does not
require the operator to provide a robot, does not require explicit hardware
configuration, and accepts offline failure statuses.

## 11. Recommendation

The immediate recommendation is to reclassify this CTest as an offline-safe SDK
integration contract test, not a pure unit test.

Engineering options, without deciding implementation here:

1. If the test must remain offline-safe, its expected statuses should include
   the exact SDK failure status that the production adapter can emit offline:
   `StatusCode::kInternalError`.

2. If `kInternalError` is considered too broad for offline SDK failures, the
   production mapping should be reviewed so non-zero SDK connection return codes
   from `GetFsmId()` become `kUnavailable` or another expected connection
   status instead of `kUnknown -> kInternalError`.

3. If the test is intended to validate real SDK communication, it should be
   moved out of default unit-style CTest execution and into an explicit
   integration or hardware validation target requiring Unitree SDK runtime,
   DDS network availability, correct network interface, and optionally a real
   Unitree G1.

Most likely defect classification:

```text
Test expectation is incomplete for the current production implementation.
```

Secondary design issue:

```text
The SDK abstraction maps non-zero SDK connection return codes to kUnknown,
which propagates as framework kInternalError. That is too coarse for an
offline/default CTest that expects unavailable/precondition connection failures.
```

