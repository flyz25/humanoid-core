# Sensor Abstraction Layer

Milestone 10.1 introduces the vendor-independent perception sensor boundary.
It defines generic sensor contracts only; it does not implement camera, LiDAR,
audio, IMU, radar, OpenCV, PCL, ROS2, Unitree, DDS, or vendor SDK integration.

## Public Headers

```cpp
#include <humanoid/perception/Sensor.h>
#include <humanoid/perception/SensorCapabilities.h>
#include <humanoid/perception/SensorFactory.h>
#include <humanoid/perception/SensorFrame.h>
#include <humanoid/perception/SensorManager.h>
#include <humanoid/perception/SensorState.h>
#include <humanoid/perception/SensorType.h>
#include <humanoid/perception/DetectionResult.h>
#include <humanoid/perception/IInferenceEngine.h>
#include <humanoid/perception/InferenceRequest.h>
#include <humanoid/perception/InferenceResult.h>
#include <humanoid/perception/ModelManager.h>
#include <humanoid/perception/PerceptionPipeline.h>
#include <humanoid/perception/SensorFusion.h>
```

All types are in `humanoid::perception` and are also available through the
`<humanoid/core.hpp>` umbrella header.

## Sensor Types

`SensorType` supports:

- `Camera`
- `DepthCamera`
- `Lidar`
- `Microphone`
- `Imu`
- `Radar`
- `Custom`

The enum is framework-owned metadata. It must not mirror vendor SDK enums or
middleware message names.

## Sensor Interface

`Sensor` is a pure abstract interface with:

- `Initialize()`
- `Shutdown()`
- `Start()`
- `Stop()`
- `ReadFrame()`
- `Capabilities()`
- `Health()`
- `Configuration()`

Implementations must contain backend exceptions and return
`humanoid::common::Status` or `SensorFrameResult`. Applications and framework
modules depend on this interface, not concrete drivers or SDK clients.

## Frames

`SensorFrame` is a byte-oriented value type. It carries:

- Producer-assigned frame id.
- Sensor type.
- Monotonic timestamp.
- Optional image-like dimensions.
- Channel and element-size metadata.
- Encoding label.
- Owned payload bytes.
- Ordered metadata values.

Frame payload bytes intentionally avoid OpenCV matrices, PCL clouds, ROS2
messages, DDS samples, vendor handles, and raw owning pointers.

## Capabilities and Configuration

`SensorCapabilities` declares sensor identity, type, supported encodings,
maximum dimensions, sample rate, timestamp support, streaming support, and
configuration support.

`SensorConfiguration` is an ordered map of scalar framework-owned values. It is
for generic configuration snapshots only and must not store SDK objects,
middleware messages, file descriptors, pointers, or vendor handles.

## Health and State

`SensorHealth` reports `Unknown`, `Healthy`, `Degraded`, `Faulted`, or
`Unavailable` state with optional diagnostic text.

`SensorState` combines lifecycle, health, produced-frame count, frame-error
count, and timestamp data.

## Factory

`SensorFactory` is a thread-safe, non-singleton creator registry:

```text
Application or perception host
  -> SensorFactory
    -> SensorCapabilities
    -> SensorCreator
      -> Sensor
```

It supports creator registration, unregistration, creation, enumeration,
containment checks, and size queries. It performs no hardware discovery,
dynamic loading, stream polling, SDK initialization, or middleware
communication.

## Manager

`SensorManager` owns active `Sensor` instances and provides a thread-safe
coordination point for:

- Sensor registration and hot-plug unregistration.
- Active sensor discovery through capability snapshots.
- Per-sensor lifecycle forwarding.
- Health and manager-maintained state snapshots.
- Serialized per-sensor frame reads.
- Manager timestamping for frames that arrive without timestamps.
- Latest-frame storage.
- Frame routing to subscribed listeners.
- Manager statistics.

Frame listeners are invoked outside manager and sensor locks. Listener
exceptions are contained and counted so one consumer cannot break frame routing
for other consumers.

The manager performs no hardware discovery, driver loading, transport
communication, polling thread creation, or vendor-specific synchronization. A
future perception service may compose `SensorFactory`, `SensorManager`, and
plugin-provided concrete sensors without changing the core perception API.

## Perception Pipeline

`PerceptionPipeline` provides a thread-safe configurable stage graph:

```text
Frame
  -> Filter
    -> Inference
      -> Detection
        -> Tracking
          -> Output
```

Stages are injected through `IPerceptionStage`. The pipeline owns graph
configuration, dependency validation, topological execution, per-pipeline
execution serialization, and result collection. Stages own their own domain
logic and may wrap framework services through dependency injection.

The pipeline does not own sensors, inference engines, trackers, SDK clients,
middleware transports, or plugin loaders.

## Inference

`IInferenceEngine` is the only inference backend boundary. Future engines may
wrap TensorRT, ONNX Runtime, Torch, OpenVINO, Ollama, cloud APIs, or custom
runtime code behind the same interface.

`InferenceRequest` carries a model id, input frame, scalar parameters, and
timestamp. `InferenceResult` carries a status, model id, named scalar/vector
outputs, optional detections, and timestamp.

`ModelManager` is a thread-safe registry for injected engines and model
metadata. It validates model-to-engine relationships and dispatches requests to
the registered engine. It performs no model parsing, filesystem loading,
runtime compilation, network download, or backend initialization policy.

## Detection

`DetectionResult` provides generic output values for:

- Object detection.
- Pose detection.
- Face detection.
- QR detection.
- Marker detection.
- Semantic segmentation.
- Custom detections.

Each result contains an id, confidence, label, 2D bounding box, optional 3D
position, timestamp, and optional tracking id. Detection results are value
types and do not depend on image matrices, point clouds, middleware messages,
or backend tensors.

## Sensor Fusion

`SensorFusion.h` provides:

- `CoordinateFrame`
- `CoordinateTransform`
- `FrameSynchronizationPolicy`
- `SynchronizedFrameSet`
- `FusionRequest`
- `FusionResult`
- `ISensorFusion`
- `FrameSynchronizer`

`FrameSynchronizer` aligns frames by monotonic timestamp and required sensor
types. `ISensorFusion` is a pure interface for future fusion implementations.
This layer intentionally excludes SLAM, localization, ROS2 TF, map building,
calibration estimation, and vendor coordinate systems.

## Dependency Boundary

```text
Application / future perception manager
  -> Sensor
  -> SensorFactory
  -> SensorManager
  -> PerceptionPipeline
  -> IInferenceEngine / ModelManager
  -> DetectionResult
  -> FrameSynchronizer / ISensorFusion
  -> SensorFrame / SensorCapabilities / SensorState
    -> humanoid::common::Status
```

The perception abstraction has no dependency on robot adapters, Unitree SDK2,
OpenCV, PCL, ROS2, DDS, camera SDKs, LiDAR SDKs, audio APIs, mission execution,
behavior trees, or AI planning.
