from collections import deque

import numpy as np
import webrtcvad

SAMPLE_RATE = 16000
FRAME_MS = 30
FRAME_BYTES = SAMPLE_RATE * FRAME_MS // 1000 * 2  # PCM16 mono


class VoiceActivityDetector:
    """Speech / silence detector on a 16 kHz mono PCM16 stream.

    Each 30 ms frame is classified by WebRTC VAD and pushed in a circular buffer covering
    `window_ms`. Speech starts when the voiced fraction of the buffer reaches `start_ratio`
    and stops when it falls to `end_ratio` or below (hysteresis, so `start_ratio > end_ratio`).

    A frame counts as voiced only if WebRTC says speech AND its RMS (int16 units) is at least
    `min_rms`, which rejects quiet noise misclassified as speech. Frames whose zero-crossing
    rate exceeds `max_zcr` (0..1, fraction of sample pairs) are also rejected: scratching and
    other broadband noise cross zero far more often than voiced speech. 0 / 1 disable the checks.
    """

    def __init__(
        self,
        aggressiveness: int = 2,
        window_ms: int = 600,
        start_ratio: float = 0.6,
        end_ratio: float = 0.15,
        min_rms: float = 0,
        max_zcr: float = 1.0,
    ):
        if not 0 <= aggressiveness <= 3:
            raise ValueError("aggressiveness must be in 0..3")
        if not 0 <= end_ratio < start_ratio <= 1:
            raise ValueError("need 0 <= end_ratio < start_ratio <= 1")
        n_frames = window_ms // FRAME_MS
        if n_frames < 1:
            raise ValueError(f"window_ms must be >= {FRAME_MS}")

        self._n_frames = n_frames
        self._vad = webrtcvad.Vad(aggressiveness)
        self._frames: deque[bool] = deque(maxlen=n_frames)
        self._start_ratio = start_ratio
        self._end_ratio = end_ratio
        self._min_rms = min_rms
        self._max_zcr = max_zcr
        self._rest = b""
        self.active = False

    @property
    def voiced_ratio(self) -> float:
        return sum(self._frames) / self._n_frames

    def reset(self) -> None:
        self._frames.clear()
        self._rest = b""
        self.active = False

    def _is_voiced(self, frame: bytes) -> bool:
        if not self._vad.is_speech(frame, SAMPLE_RATE):
            return False
        if self._min_rms or self._max_zcr < 1.0:
            samples = np.frombuffer(frame, dtype=np.int16).astype(np.float32)
            if self._min_rms and np.sqrt(np.mean(samples ** 2)) < self._min_rms:
                return False
            if self._max_zcr < 1.0 and np.mean(np.signbit(samples[1:]) != np.signbit(samples[:-1])) > self._max_zcr:
                return False
        return True

    def process(self, pcm: bytes) -> bool:
        """Feed PCM16 LE bytes of any length. Returns the current speech state."""
        data = self._rest + pcm
        end = len(data) - len(data) % FRAME_BYTES
        for i in range(0, end, FRAME_BYTES):
            frame = data[i:i + FRAME_BYTES]
            self._frames.append(self._is_voiced(frame))
            ratio = self.voiced_ratio
            if not self.active and ratio >= self._start_ratio:
                self.active = True
            elif self.active and ratio <= self._end_ratio:
                self.active = False
        self._rest = data[end:]
        return self.active
