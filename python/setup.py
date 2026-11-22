from glob import glob

from setuptools import find_packages, setup

package_name = "tactigon"

setup(
    name=package_name,
    version="0.1.0",
    packages=find_packages(include=["tactigon", "tactigon.*"]),
    data_files=[
        ("share/ament_index/resource_index/packages", [f"resource/{package_name}"]),
        (f"share/{package_name}", ["package.xml"]),
        (f"share/{package_name}/launch", glob("launch/*.launch.py")),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    entry_points={
        "console_scripts": [
            "ble_node = tactigon.ros.ble_node:main",
            "voice_detection_node = tactigon.ros.voice_detection_node:main",
            "transcribe_node = tactigon.ros.transcribe_node:main",
        ],
    },
)
