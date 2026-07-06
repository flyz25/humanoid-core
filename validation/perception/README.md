# Perception Validation

Perception validation verifies optional perception backends through
framework-owned interfaces. The core framework remains independent of OpenCV,
PCL, TensorRT, ONNX Runtime, and ROS2.

## Required Areas

- Camera.
- YOLO backend.
- Face detection.
- Pose detection.
- QR detection.
- AprilTag detection.
- Sensor fusion.
- Normal lighting.
- Dark environment.
- Backlight.
- Outdoor environment.

## Procedure

1. Confirm site privacy policy and data retention rules.
2. Capture frame metadata for each lighting condition.
3. Run approved detection or fusion adapters where available.
4. Compare detection results against an operator truth set.
5. Record confidence, labels, timestamps, tracking IDs, and dropped frames.
