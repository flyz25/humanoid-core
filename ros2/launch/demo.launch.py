from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("demo_profile", default_value="stage"),
            DeclareLaunchArgument("bridge_node_name", default_value="humanoid_core_bridge"),
            LogInfo(msg="humanoid-core ROS2 demo launch prepared"),
        ]
    )
