import numpy as np
import rclpy
from geometry_msgs.msg import Vector3Stamped
from rclpy.node import Node
from std_msgs.msg import Int16MultiArray

from tactigon.hal.ble import BLE
from tactigon.models import Wand
from tactigon.ros import AUDIO_QOS, WAND_QOS


class BleNode(Node):
    """Connects to the Tactigon and publishes `wand` and `audio`."""

    def __init__(self):
        super().__init__("ble_node")
        self.declare_parameter("address", "")
        address = self.get_parameter("address").value
        if not address:
            raise ValueError("parameter 'address' is required (BLE MAC address)")

        self._wand_pub = self.create_publisher(Vector3Stamped, "wand", WAND_QOS)
        self._audio_pub = self.create_publisher(Int16MultiArray, "audio", AUDIO_QOS)

        self._ble = BLE(address, wand_callback=self._on_wand, audio_callback=self._on_audio)
        self._ble.start()

    def _on_wand(self, wand: Wand) -> None:
        if not rclpy.ok():
            return
        msg = Vector3Stamped()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.vector.x = float(wand.roll)
        msg.vector.y = float(wand.pitch)
        self._wand_pub.publish(msg)

    def _on_audio(self, pcm: bytes) -> None:
        if not rclpy.ok():
            return
        msg = Int16MultiArray()
        msg.data = np.frombuffer(pcm, dtype=np.int16)
        self._audio_pub.publish(msg)

    def destroy_node(self):
        self._ble.stop()
        self._ble.join(timeout=5)
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = BleNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
