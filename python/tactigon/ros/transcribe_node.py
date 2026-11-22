import numpy as np
import rclpy
from rclpy.node import Node
from std_msgs.msg import Bool, Int16MultiArray, String

from tactigon.ros import AUDIO_QOS, LATCHED_QOS, TEXT_QOS
from tactigon.transcribe import LiveTranscriber


class TranscribeNode(Node):
    """Live transcription of `audio` while `speech_active` is true.

    Publishes `transcription_partial` (text so far) during speech and `transcription`
    (full text) when speech ends.

    Parameters:
      language    en, de, fr, es, it, nl, pl; empty or "auto" = auto-detect
      weights     path to Whistle .cact weights; empty = default model
      keywords    list of words/phrases to bias the decoder
      chunk_ms    audio sent to Whistle per step
      preroll_ms  audio kept from before the speech_active=true signal
    """

    def __init__(self):
        super().__init__("transcribe_node")
        self.declare_parameter("language", "auto")
        self.declare_parameter("weights", "")  # empty = default model
        self.declare_parameter("keywords", [""])
        self.declare_parameter("chunk_ms", 1000)
        self.declare_parameter("preroll_ms", 800)

        self._partial_pub = self.create_publisher(String, "transcription_partial", TEXT_QOS)
        self._final_pub = self.create_publisher(String, "transcription", TEXT_QOS)

        language = self.get_parameter("language").value
        self.get_logger().info(f"Language: {language}") 
        self.get_logger().info("Loading Whistle model...")
        self._transcriber = LiveTranscriber(
            weights=self.get_parameter("weights").value or None,
            language=None if language in ("", "auto") else language,
            keywords=[k for k in self.get_parameter("keywords").value if k],
            chunk_ms=self.get_parameter("chunk_ms").value,
            preroll_ms=self.get_parameter("preroll_ms").value,
            on_partial=self._on_partial,
            on_final=self._on_final,
        )
        self._transcriber.start()

        self.create_subscription(Int16MultiArray, "audio", self._on_audio, AUDIO_QOS)
        self.create_subscription(Bool, "speech_active", self._on_speech_active, LATCHED_QOS)
        self.get_logger().info("Ready")

    def _on_audio(self, msg: Int16MultiArray) -> None:
        self._transcriber.feed(np.frombuffer(msg.data, dtype=np.int16))

    def _on_speech_active(self, msg: Bool) -> None:
        self._transcriber.set_active(msg.data)

    def _on_partial(self, text: str) -> None:
        if rclpy.ok():
            self._partial_pub.publish(String(data=text))

    def _on_final(self, text: str) -> None:
        self.get_logger().info(f"Transcription: {text}")
        if rclpy.ok():
            self._final_pub.publish(String(data=text))

    def destroy_node(self):
        self._transcriber.stop()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = TranscribeNode()
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
