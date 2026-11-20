class ADPCMEngine:
    STEP_TABLE = [
         7,  8,  9, 10, 11, 12, 13, 14,
        16, 17, 19, 21, 23, 25, 28, 31,
        34, 37, 41, 45, 50, 55, 60, 66,
        73, 80, 88, 97,107,118,130,143,
       157,173,190,209,230,253,279,307,
       337,371,408,449,494,544,598,658,
       724,796,876,963,1060,1166,1282,1411,
      1552,1707,1878,2066,2272,2499,2749,3024,
      3327,3660,4026,4428,4871,5358,5894,6484,
      7132,7845,8630,9493,10442,11487,12635,13899,
      15289,16818,18500,20350,22385,24623,27086,29794,
      32767
    ]

    INDEX_TABLE = [
        -1, -1, -1, -1,
         2,  4,  6,  8,
        -1, -1, -1, -1,
         2,  4,  6,  8
    ]

    def __init__(self):
        self.reset()

    def reset(self):
        self.prev_sample = 0
        self.index = 0
        self._pending_nibble = None  # nibble rimasto dal chunk precedente

    def _decode_nibble(self, delta: int) -> int:
        """Decodifica un singolo nibble ADPCM → PCM"""
        step = self.STEP_TABLE[self.index]

        diffq = step >> 3
        if delta & 4:
            diffq += step
        if delta & 2:
            diffq += step >> 1
        if delta & 1:
            diffq += step >> 2

        if delta & 8:
            self.prev_sample -= diffq
        else:
            self.prev_sample += diffq

        # Saturazione int16
        if self.prev_sample > 32767:
            self.prev_sample = 32767
        elif self.prev_sample < -32768:
            self.prev_sample = -32768

        # Aggiorna index
        self.index += self.INDEX_TABLE[delta & 0x0F]
        if self.index < 0:
            self.index = 0
        elif self.index > 88:
            self.index = 88

        return self.prev_sample

    def decode_chunk(self, data: bytearray) -> list:
        """
        Decodifica un chunk ADPCM (BLE / seriale).
        Può essere chiamata con chunk di dimensione arbitraria.

        - data: bytes ricevuti
        - out: lista opzionale da riempire (no allocazioni)
        """
        out = []

        for byte in data:
            low = byte & 0x0F
            high = (byte >> 4) & 0x0F

            out.append(self._decode_nibble(low))
            out.append(self._decode_nibble(high))

        return out
