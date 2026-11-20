"""ROS 2 nodes. Topics (relative, run them in the same namespace, e.g. `tactigon`):

  audio                  std_msgs/Int16MultiArray   16 kHz mono PCM16 (ble_node -> others)
  wand                   geometry_msgs/Vector3Stamped  x=roll, y=pitch [rad]
  speech_active          std_msgs/Bool              VAD state, published on change (latched)
  transcription_partial  std_msgs/String            text so far of the running utterance
  transcription          std_msgs/String            full text, once the utterance ends
"""
from rclpy.qos import (
    DurabilityPolicy,
    HistoryPolicy,
    QoSProfile,
    ReliabilityPolicy,
    qos_profile_sensor_data,
)

AUDIO_QOS = QoSProfile(
    reliability=ReliabilityPolicy.RELIABLE,
    history=HistoryPolicy.KEEP_LAST,
    depth=200,
)
WAND_QOS = qos_profile_sensor_data
LATCHED_QOS = QoSProfile(
    reliability=ReliabilityPolicy.RELIABLE,
    durability=DurabilityPolicy.TRANSIENT_LOCAL,
    history=HistoryPolicy.KEEP_LAST,
    depth=1,
)
TEXT_QOS = QoSProfile(
    reliability=ReliabilityPolicy.RELIABLE,
    history=HistoryPolicy.KEEP_LAST,
    depth=10,
)
