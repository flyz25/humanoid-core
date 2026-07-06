from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("log_level", default_value="info"),
            DeclareLaunchArgument("bridge_node_name", default_value="humanoid_core_bridge"),
            LogInfo(msg="humanoid-core ROS2 development launch prepared"),
        ]
    )
