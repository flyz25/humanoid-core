# Unitree SDK Abstraction

This directory is the only production boundary that may include Unitree SDK2
headers. The abstraction converts Unitree SDK calls, return codes, exceptions,
and normalized robot state into humanoid-core owned types.

## Dependency Flow

```text
UnitreeG1Adapter
  -> LocoClientWrapper
    -> SdkWrapper
      -> Unitree SDK2
```

`LocoClientWrapper` exists as a compatibility facade for the Milestone 2
adapter-facing API. It does not include Unitree SDK2 headers.

## Files

- `SdkWrapper.h` / `SdkWrapper.cpp`: RAII SDK boundary for initialization,
  shutdown, discovery, connection, disconnection, and locomotion commands.
- `SdkTypes.h`: normalized SDK abstraction result, state, descriptor, and enum
  types.
- `SdkConverter.h` / `SdkConverter.cpp`: conversions from abstraction types to
  framework `Result`, adapter connection state, and `humanoid::core::RobotState`.

## Build Note

The public framework remains C++20. `SdkWrapper.cpp` is compiled with the
vendor-compatible dialect required by the official Unitree SDK2 headers. This is
contained inside the SDK boundary and does not affect public framework targets.
