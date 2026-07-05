# Milestone 10 Report: Perception Framework

Milestone 10 completes the vendor-independent perception architecture for
humanoid-core.

## Scope

Milestone 10 adds:

- Generic sensor abstraction.
- Active sensor manager.
- Perception pipeline graph.
- Inference engine abstraction.
- Model manager.
- Detection result framework.
- Sensor fusion contracts.
- Timestamp-based frame synchronization.
- Runnable perception examples.

## Architecture

```text
Application / future perception service
  -> SensorFactory
  -> SensorManager
    -> Sensor
      -> SensorFrame
  -> PerceptionPipeline
    -> IPerceptionStage
      -> IInferenceEngine / ModelManager
      -> DetectionResult
      -> FrameSynchronizer / ISensorFusion
```

The framework owns interfaces, value types, registries, graph execution, and
timestamp alignment only. Concrete camera drivers, LiDAR drivers, inference
backends, trackers, fusion algorithms, and vendor SDKs remain outside the core
framework and must be injected.

## Dependency Rules

Perception code must not depend on:

- ROS2
- DDS
- OpenCV
- PCL
- TensorRT
- ONNX Runtime
- Torch
- OpenVINO
- Unitree SDK
- Vendor sensor SDKs

Applications depend on framework interfaces and receive concrete
implementations through dependency injection.

## Components

`Sensor`, `SensorFactory`, and `SensorManager` provide sensor lifecycle,
registration, hot-plug removal, health/state snapshots, synchronized reads,
timestamping, latest-frame storage, frame routing, and statistics.

`PerceptionPipeline` provides a configurable graph for frame, filter,
inference, detection, tracking, and output stages.

`IInferenceEngine`, `InferenceRequest`, `InferenceResult`, and `ModelManager`
provide replaceable inference backend boundaries without backend dependencies.

`DetectionResult` provides generic output for object, pose, face, QR, marker,
semantic segmentation, and custom detections.

`SensorFusion.h` provides coordinate metadata, synchronization policy, fusion
request/result values, `ISensorFusion`, and `FrameSynchronizer`.

## Examples

Added examples:

- `humanoid_core_perception_camera_example`
- `humanoid_core_perception_detection_example`
- `humanoid_core_perception_inference_example`
- `humanoid_core_perception_pipeline_example`
- `humanoid_core_perception_sensor_fusion_example`

## Validation

Milestone validation covers:

- Debug build.
- Release build.
- `ENABLE_UNITREE=ON`.
- `ENABLE_UNITREE=OFF`.
- CTest.
- Install.
- CPack package generation.
- Example compilation.
- Documentation linting.

## Production Status

Milestone 10 is architecture-complete for perception. Runtime integration with
physical sensors, inference backends, tracking algorithms, SLAM, mapping, ROS2,
or vendor SDKs remains intentionally out of scope.
