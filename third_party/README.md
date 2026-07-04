# Third-Party Dependencies

`unitree_sdk2` is the only SDK submodule currently used by this repository.

```text
Path: third_party/unitree_sdk2
Repository: https://github.com/unitreerobotics/unitree_sdk2.git
Pinned tag: 2.0.2
Pinned commit: 811bc77
```

The SDK is consumed in-place by CMake through `cmake/FindUnitreeSDK2.cmake`.
Do not install it into `/usr/local` for this project.

No ROS2, OpenCV, AI runtime, mission engine, planner, navigation framework, or
GUI dependency belongs in this repository layer.
