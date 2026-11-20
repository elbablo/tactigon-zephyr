from collections import deque

import webrtcvad

SAMPLE_RATE = 16000
FRAME_MS = 30
FRAME_BYTES = SAMPLE_RATE * FRAME_MS // 1000 * 2  # PCM16 mono


class VoiceActivityDetector:
    """Speech / silence detector on a 16 kHz mono PCM16 stream.

    Each 30 ms frame is classified by WebRTC VAD and pushed in a circular buffer covering
    `window_ms`. Speech starts when the voiced fraction of the buffer reaches `start_ratio`
    and stops when it falls to `end_ratio` or below (hysteresis, so `start_ratio > end_ratio`).
    """

    def __init__(
        self,
        aggressiveness: int = 2,
        window_ms: int = 600,
        start_ratio: float = 0.6,
        end_ratio: float = 0.15,
    ):
        if not 0 <= aggressiveness <= 3:
            raise ValueError("aggressiveness must be in 0..3")
        if not 0 <= end_ratio < start_ratio <= 1:
            raise ValueError("need 0 <= end_ratio < start_ratio <= 1")
        n_frames = window_ms // FRAME_MS
        if n_frames < 1:
            raise ValueError(f"window_ms must be >= {FRAME_MS}")

        self._vad = webrtcvad.Vad(aggressiveness)
        self._frames: deque[bool] = deque(maxlen=n_frames)
        self._start_ratio = start_ratio
        self._end_ratio = end_ratio
        self._rest = b""
        self.active = False

    @property
    def voiced_ratio(self) -> float:
        return sum(self._frames) / self._frames.maxlen

    def reset(self) -> None:
        self._frames.clear()
        self._rest = b""
        self.active = False

    def process(self, pcm: bytes) -> bool:
        """Feed PCM16 LE bytes of any length. Returns the current speech state."""
        data = self._rest + pcm
        end = len(data) - len(data) % FRAME_BYTES
        for i in range(0, end, FRAME_BYTES):
            self._frames.append(self._vad.is_speech(data[i:i + FRAME_BYTES], SAMPLE_RATE))
            ratio = self.voiced_ratio
            if not self.active and ratio >= self._start_ratio:
                self.active = True
            elif self.active and ratio <= self._end_ratio:
                self.active = False
        self._rest = data[end:]
        return self.active
