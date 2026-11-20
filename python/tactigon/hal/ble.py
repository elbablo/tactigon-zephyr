import asyncio
import logging
import struct
from typing import Callable
from bleak import BleakClient
from threading import Thread, Event
from queue import Queue

from tactigon.models import Acceleration, Gyro, Wand
from tactigon.adpcm.adpcm import ADPCMEngine

WandCallback = Callable[[Wand], None]
AudioCallback = Callable[[bytes], None]


class BLE(Thread):
    """Connects to the Tactigon, decodes the ADPCM audio and parses the IMU packets.

    Audio is 16 kHz mono PCM16 little-endian bytes, put on `audio_queue` and passed to
    `audio_callback` (if given). Callbacks run in the BLE thread.
    """

    address: str
    stop_event: Event
    client: BleakClient | None = None

    IMU_CHAR_UUID = "100f0e0d-0c0b-0a09-0807-060504030201"
    AUDIO_CHAR_UUID = "201f1e1d-1c1b-1a19-1817-161514131211"

    RECONNECT_DELAY_S = 2.0

    def __init__(
        self,
        address: str,
        wand_callback: WandCallback | None = None,
        audio_callback: AudioCallback | None = None,
    ):
        Thread.__init__(self, daemon=True)
        self.logger = logging.getLogger(BLE.__name__)
        self.address = address
        self.wand_callback = wand_callback
        self.audio_callback = audio_callback
        self.adpcm = ADPCMEngine()
        self.audio_queue = Queue()
        self.stop_event = Event()

    def __enter__(self):
        self.start()
        return self
    
    def __exit__(self, *args, **kwargs):
        self.stop()
        self.join()

    def run(self):
        self.stop_event.clear()
        asyncio.run(self.run_ble())

    def imu_cb(self, char, data: bytearray):
        # Wire order set by firmware (imu_ble_cb): pitch first, then roll.
        ax, ay, az, gx, gy, gz, pitch, roll = struct.unpack_from("<hhhhhhff", data)

        acc = Acceleration(ax / 1000, ay / 1000, az / 1000)  # g
        gyro = Gyro(gx / 1000, gy / 1000, gz / 1000)  # rad/s
        wand = Wand(roll, pitch)  # rad

        if self.wand_callback:
            self.wand_callback(wand)

    def audio_cb(self, char, data: bytearray):
        pcm = self.adpcm.decode_chunk(data)
        pcm_bytes = struct.pack("<{}h".format(len(pcm)), *pcm)
        self.audio_queue.put_nowait(pcm_bytes)
        if self.audio_callback:
            self.audio_callback(pcm_bytes)

    async def run_ble(self):
        while not self.stop_event.is_set():
            loop = asyncio.get_running_loop()
            disconnected = asyncio.Event()
            self.client = BleakClient(
                self.address,
                disconnected_callback=lambda _: loop.call_soon_threadsafe(disconnected.set),
            )

            try:
                self.logger.info("Connecting to %s", self.address)
                await self.client.connect()
                self.logger.info("Connected!")
                self.adpcm.reset()
                await self.client.start_notify(self.IMU_CHAR_UUID, self.imu_cb)
                self.logger.info("IMU notify started...")
                await self.client.start_notify(self.AUDIO_CHAR_UUID, self.audio_cb)
                self.logger.info("AUDIO notify started...")

                while not self.stop_event.is_set() and not disconnected.is_set():
                    await asyncio.sleep(0.02)
            except Exception as e:
                self.logger.warning("BLE error: %s", e)
            finally:
                try:
                    await self.client.disconnect()
                except Exception:
                    pass
                self.logger.info("Disconnected!")

            if not self.stop_event.is_set():
                await asyncio.sleep(self.RECONNECT_DELAY_S)

    def stop(self):
        self.stop_event.set()