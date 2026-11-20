"""Launch the three nodes with the project venv.

  source /opt/ros/jazzy/setup.bash
  ros2 launch python/launch/tactigon.launch.py address:=F3:5D:DF:89:26:7B language:=it
"""
import os

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration

PYTHON_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
VENV_PYTHON = os.path.join(PYTHON_DIR, "..", ".venv", "bin", "python")
NS = "tactigon"


def node(module: str, name: str, params: dict) -> ExecuteProcess:
    ros_args = ["--ros-args", "-r", f"__ns:=/{NS}", "-r", f"__node:={name}"]
    for key, value in params.items():
        ros_args += ["-p", [f"{key}:=", value]]
    return ExecuteProcess(
        cmd=[VENV_PYTHON, "-m", f"tactigon.ros.{module}", *ros_args],
        cwd=PYTHON_DIR,
        output="screen",
    )


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument("address", description="BLE MAC address of the Tactigon"),
        DeclareLaunchArgument("language", default_value="auto", description="en/de/fr/es/it/nl/pl or auto"),
        DeclareLaunchArgument("window_ms", default_value="600"),
        DeclareLaunchArgument("start_ratio", default_value="0.6"),
        DeclareLaunchArgument("end_ratio", default_value="0.15"),
        node("ble_node", "ble_node", {"address": LaunchConfiguration("address")}),
        node("activity_detection_node", "activity_detection_node", {
            "window_ms": LaunchConfiguration("window_ms"),
            "start_ratio": LaunchConfiguration("start_ratio"),
            "end_ratio": LaunchConfiguration("end_ratio"),
        }),
        node("transcribe_node", "transcribe_node", {"language": LaunchConfiguration("language")}),
    ])
