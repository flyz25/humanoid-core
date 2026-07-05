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

## Dependency Boundary

```text
Application / future perception manager
  -> Sensor
  -> SensorFactory
  -> SensorManager
  -> SensorFrame / SensorCapabilities / SensorState
    -> humanoid::common::Status
```

The perception abstraction has no dependency on robot adapters, Unitree SDK2,
OpenCV, PCL, ROS2, DDS, camera SDKs, LiDAR SDKs, audio APIs, mission execution,
behavior trees, or AI planning.
