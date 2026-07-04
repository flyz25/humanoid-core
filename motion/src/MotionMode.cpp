#include <humanoid/motion/MotionMode.hpp>

namespace humanoid::motion {

std::string_view toString(MotionMode mode) noexcept {
  switch (mode) {
  case MotionMode::kIdle:
    return "idle";
  case MotionMode::kHoldPosition:
    return "hold_position";
  case MotionMode::kJointPosition:
    return "joint_position";
  case MotionMode::kJointVelocity:
    return "joint_velocity";
  case MotionMode::kCartesianVelocity:
    return "cartesian_velocity";
  }

  return "unknown";
}

} // namespace humanoid::motion
