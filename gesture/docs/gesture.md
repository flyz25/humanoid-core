# Gesture Module

The gesture module defines the abstract gesture controller boundary and a
manager for delegating named gesture requests. It intentionally does not define
gesture libraries, animation blending, vendor gesture APIs, or motion execution
details.

Gesture implementations must remain behind `GestureController`.
