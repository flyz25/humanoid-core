# Architecture

humanoid-core uses Clean Architecture with vendor SDKs isolated behind adapter
plugins and SDK wrappers.

```text
Applications
  -> Managers
    -> Interfaces
      -> Robot Factory
        -> Robot Adapter
          -> SDK Wrapper
            -> Vendor SDK
```

Applications receive adapters through `RobotFactoryRegistry` and depend only on
`IRobotFactory` and `IRobotAdapter`.

## Unitree G1

```text
Application
      -> RobotFactoryRegistry
        -> UnitreeRobotFactory
          -> UnitreeG1Adapter
            -> LocoClientWrapper
              -> SdkWrapper
                -> Unitree SDK2
```

`plugins/unitree/sdk/SdkWrapper.cpp` is the only production translation unit
allowed to include Unitree SDK2 headers. `LocoClientWrapper` is a compatibility
facade over that abstraction. `UnitreeG1Adapter` translates generic framework
commands and delegates to the wrapper. It contains no mission, behavior,
planning, navigation, or application logic.

Unitree SDK2 is pinned as a submodule:

```text
third_party/unitree_sdk2
tag: 2.0.2
commit: 811bc77
```
