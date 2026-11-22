import logging
from queue import Empty

import numpy as np

from tactigon.hal.ble import BLE
from tactigon.transcribe import LiveTranscriber
from tactigon.vad import VoiceActivityDetector

logging.basicConfig(level=logging.INFO)

ADDRESS = "F3:5D:DF:89:26:7B"
LANGUAGE = "it"  # None = auto-detect


def on_partial(text: str) -> None:
    print(f"\r\033[K... {text}", end="", flush=True)


def on_final(text: str) -> None:
    print(f"\r\033[K> {text}", flush=True)


if __name__ == "__main__":
    # Parametri piu' severi per scartare rumori (unghie sul tavolo, ecc.); da tarare.
    # Finestra corta + soglia piu' bassa: servono ~240 ms di voce per partire, cosi' passano anche "ciao".
    vad = VoiceActivityDetector(aggressiveness=3, window_ms=400, start_ratio=0.6, min_rms=500, max_zcr=0.25)
    transcriber = LiveTranscriber(language=LANGUAGE, on_partial=on_partial, on_final=on_final)
    transcriber.start()

    try:
        with BLE(ADDRESS) as b:
            while True:
                try:
                    chunk = b.audio_queue.get(timeout=0.1)
                except Empty:
                    continue

                # Il VAD decide inizio/fine frase; il trascrittore riceve sempre l'audio
                # (serve per il preroll) e trascrive solo mentre e' attivo.
                transcriber.feed(np.frombuffer(chunk, dtype=np.int16))
                transcriber.set_active(vad.process(chunk))
    except KeyboardInterrupt:
        pass
    finally:
        transcriber.stop()
