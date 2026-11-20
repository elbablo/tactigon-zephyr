import logging
from collections import deque
from queue import Queue
from threading import Lock, Thread
from typing import Callable, Iterator

import numpy as np

SAMPLE_RATE = 16000

TextCallback = Callable[[str], None]


def _drain(q: "Queue[np.ndarray | None]") -> Iterator[np.ndarray]:
    while (chunk := q.get()) is not None:
        yield chunk


class LiveTranscriber:
    """Live speech-to-text with Cactus Whistle, driven by an external speech/silence signal.

    Call `feed()` with every PCM16 16 kHz mono block and `set_active()` when speech starts/stops.
    While active, audio goes to Whistle in `chunk_ms` pieces and `on_partial` gets the text so far
    (committed + pending). When speech stops, `on_final` gets the whole utterance.
    `preroll_ms` of audio from before the start signal is included, since the VAD reacts late.

    Utterances are transcribed one at a time, in a worker thread. Callbacks run in that thread.
    """

    def __init__(
        self,
        weights: str | None = None,
        language: str | None = None,
        keywords: list[str] | None = None,
        chunk_ms: int = 1000,
        preroll_ms: int = 800,
        on_partial: TextCallback | None = None,
        on_final: TextCallback | None = None,
    ):
        from cactus.agent.whistle import Whistle

        self.logger = logging.getLogger(LiveTranscriber.__name__)
        self.language = language or None
        self.keywords = keywords or None
        self.on_partial = on_partial
        self.on_final = on_final
        self._whistle = Whistle(weights=weights)

        self._chunk_samples = SAMPLE_RATE * chunk_ms // 1000
        self._preroll_samples = SAMPLE_RATE * preroll_ms // 1000
        self._preroll: deque[np.ndarray] = deque()
        self._preroll_count = 0

        self._lock = Lock()
        self._active = False
        self._current: "Queue[np.ndarray | None] | None" = None
        self._pending: list[np.ndarray] = []
        self._pending_count = 0

        self._jobs: "Queue[Queue[np.ndarray | None] | None]" = Queue()
        self._worker = Thread(target=self._run, name="LiveTranscriber", daemon=True)

    def start(self) -> None:
        self._worker.start()

    def stop(self) -> None:
        self.set_active(False)
        self._jobs.put(None)
        self._worker.join()

    def feed(self, pcm: np.ndarray) -> None:
        """pcm: int16 samples at 16 kHz mono."""
        with self._lock:
            if self._active:
                self._append(pcm)
            else:
                self._preroll.append(pcm)
                self._preroll_count += len(pcm)
                while len(self._preroll) > 1 and self._preroll_count - len(self._preroll[0]) >= self._preroll_samples:
                    self._preroll_count -= len(self._preroll.popleft())

    def set_active(self, active: bool) -> None:
        with self._lock:
            if active == self._active:
                return
            self._active = active
            if active:
                self._current = Queue()
                self._jobs.put(self._current)
                preroll, self._preroll, self._preroll_count = list(self._preroll), deque(), 0
                for block in preroll:
                    self._append(block)
            else:
                self._flush()
                self._current.put(None)
                self._current = None

    def _append(self, pcm: np.ndarray) -> None:
        self._pending.append(pcm)
        self._pending_count += len(pcm)
        if self._pending_count >= self._chunk_samples:
            self._flush(self._chunk_samples)

    def _flush(self, limit: int | None = None) -> None:
        """Send pending audio to the current utterance, `limit` samples per chunk (all if None)."""
        if not self._pending:
            return
        audio = np.concatenate(self._pending)
        step = limit or len(audio)
        n = len(audio) - len(audio) % step if limit else len(audio)
        for i in range(0, n, step):
            self._current.put(audio[i:i + step].astype(np.float32) / 32768.0)
        rest = audio[n:]
        self._pending = [rest] if len(rest) else []
        self._pending_count = len(rest)

    def _run(self) -> None:
        while (utterance := self._jobs.get()) is not None:
            committed: list[str] = []
            last_partial = ""
            try:
                for result in self._whistle.stream(
                    _drain(utterance), language=self.language, keywords=self.keywords
                ):
                    text = result.get("text", "").strip()
                    if text:
                        committed.append(text)
                    pending = result.get("pending", "").strip()
                    partial = " ".join(committed + ([pending] if pending else []))
                    if partial and partial != last_partial and self.on_partial:
                        self.on_partial(partial)
                    last_partial = partial
            except Exception:
                self.logger.exception("Transcription failed")

            final = " ".join(committed)
            if final and self.on_final:
                self.on_final(final)
