from dataclasses import dataclass

@dataclass
class Acceleration:
    x: float
    y: float
    z: float

@dataclass
class Gyro:
    x: float
    y: float
    z: float

@dataclass
class Wand:
    roll: float
    pitch: float