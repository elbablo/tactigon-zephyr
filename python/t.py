import logging
import wave
import time
from bleak import BleakScanner

from tactigon.hal.ble import BLE

logging.basicConfig(level=logging.INFO)

if __name__ == "__main__":
    with BLE("F3:5D:DF:89:26:7B") as b:
        while True:
            try:
                chunk = b.audio_queue.get_nowait()
            except:
                time.sleep(0.1)
                continue
            
            with wave.open("test.wav", "wb") as w_f:
                w_f.setframerate(16000)
                w_f.setnchannels(1)
                w_f.setsampwidth(2)

                w_f.writeframes(chunk)

                while True:
                    try:
                        chunk = b.audio_queue.get_nowait()
                    except:
                        time.sleep(0.1)
                        continue

                    w_f.writeframes(chunk)