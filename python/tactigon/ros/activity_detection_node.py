import numpy as np
import rclpy
from rclpy.node import Node
from std_msgs.msg import Bool, Int16MultiArray

from tactigon.ros import AUDIO_QOS, LATCHED_QOS
from tactigon.vad import VoiceActivityDetector


class ActivityDetectionNode(Node):
    """Subscribes to `audio`, publishes the speech/silence state on `speech_active` (on change).

    Parameters:
      aggressiveness  WebRTC VAD mode 0..3 (3 = filters non-speech most)
      window_ms       length of the circular buffer of VAD decisions
      start_ratio     speech starts when this fraction of the buffer is voiced
      end_ratio       speech ends when the voiced fraction falls to this value or below
    """

    def __init__(self):
        super().__init__("activity_detection_node")
        self.declare_parameter("aggressiveness", 2)
        self.declare_parameter("window_ms", 600)
        self.declare_parameter("start_ratio", 0.6)
        self.declare_parameter("end_ratio", 0.15)

        self._vad = VoiceActivityDetector(
            aggressiveness=self.get_parameter("aggressiveness").value,
            window_ms=self.get_parameter("window_ms").value,
            start_ratio=self.get_parameter("start_ratio").value,
            end_ratio=self.get_parameter("end_ratio").value,
        )

        self._pub = self.create_publisher(Bool, "speech_active", LATCHED_QOS)
        self.create_subscription(Int16MultiArray, "audio", self._on_audio, AUDIO_QOS)
        self._publish(False)

    def _publish(self, active: bool) -> None:
        self._last = active
        self._pub.publish(Bool(data=active))
        self.get_logger().info("speech" if active else "silence")

    def _on_audio(self, msg: Int16MultiArray) -> None:
        pcm = np.frombuffer(msg.data, dtype=np.int16).tobytes()
        active = self._vad.process(pcm)
        if active != self._last:
            self._publish(active)


def main(args=None):
    rclpy.init(args=args)
    node = ActivityDetectionNode()
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
