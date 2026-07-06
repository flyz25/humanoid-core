from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("simulation_backend", default_value="external"),
            DeclareLaunchArgument("bridge_node_name", default_value="humanoid_core_bridge"),
            LogInfo(msg="humanoid-core ROS2 simulation launch prepared"),
        ]
    )
