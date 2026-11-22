"""Launch the three nodes (activate the project venv first).

  source /opt/vulcanexus/jazzy/setup.bash
  source .venv/bin/activate
  cd python && colcon build --symlink-install && source install/setup.bash
  ros2 launch tactigon tactigon.launch.py address:=F3:5D:DF:89:26:7B language:=it
"""
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

NS = "tactigon"


def node(executable: str, params: dict) -> Node:
    return Node(
        package="tactigon",
        executable=executable,
        name=executable,
        namespace=NS,
        output="screen",
        parameters=[params],
    )


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument("address", description="BLE MAC address of the Tactigon"),
        DeclareLaunchArgument("language", default_value="auto", description="en/de/fr/es/it/nl/pl or auto"),
        DeclareLaunchArgument("window_ms", default_value="600"),
        DeclareLaunchArgument("start_ratio", default_value="0.6"),
        DeclareLaunchArgument("end_ratio", default_value="0.15"),
        node("ble_node", {
            "address": ParameterValue(LaunchConfiguration("address"), value_type=str),
        }),
        node("voice_detection_node", {
            "window_ms": ParameterValue(LaunchConfiguration("window_ms"), value_type=int),
            "start_ratio": ParameterValue(LaunchConfiguration("start_ratio"), value_type=float),
            "end_ratio": ParameterValue(LaunchConfiguration("end_ratio"), value_type=float),
        }),
        node("transcribe_node", {
            "language": ParameterValue(LaunchConfiguration("language"), value_type=str),
        }),
    ])
