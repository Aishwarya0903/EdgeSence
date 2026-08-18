"""UART-style framing shared by the TCP edge gateway."""
from __future__ import annotations

from dataclasses import dataclass
import struct

SOF = b"\xAA\x55"
VERSION = 1
TELEMETRY_TYPE = 1
MAX_PAYLOAD = 128
HEADER_FORMAT = "<2sBBH"
PAYLOAD_FORMAT = "<HIQfff"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
PAYLOAD_SIZE = struct.calcsize(PAYLOAD_FORMAT)


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


@dataclass(frozen=True)
class Telemetry:
    node_id: int
    sequence: int
    timestamp_ms: int
    temperature_c: float
    humidity_pct: float
    vibration_g: float


def encode_telemetry(reading: Telemetry) -> bytes:
    payload = struct.pack(PAYLOAD_FORMAT, reading.node_id, reading.sequence, reading.timestamp_ms,
                          reading.temperature_c, reading.humidity_pct, reading.vibration_g)
    header = struct.pack(HEADER_FORMAT, SOF, VERSION, TELEMETRY_TYPE, len(payload))
    return header + payload + struct.pack("<H", crc16_ccitt(header[2:] + payload))


class FrameDecoder:
    """Incremental, resynchronizing decoder for fragmented TCP streams."""
    def __init__(self) -> None:
        self._buffer = bytearray()

    def feed(self, data: bytes) -> list[Telemetry]:
        self._buffer.extend(data)
        decoded: list[Telemetry] = []
        while True:
            start = self._buffer.find(SOF)
            if start < 0:
                self._buffer[:] = self._buffer[-1:]
                break
            if start:
                del self._buffer[:start]
            if len(self._buffer) < HEADER_SIZE:
                break
            _, version, msg_type, length = struct.unpack_from(HEADER_FORMAT, self._buffer)
            if version != VERSION or msg_type != TELEMETRY_TYPE or length != PAYLOAD_SIZE or length > MAX_PAYLOAD:
                del self._buffer[0]
                continue
            total = HEADER_SIZE + length + 2
            if len(self._buffer) < total:
                break
            body = bytes(self._buffer[2:HEADER_SIZE + length])
            expected = struct.unpack_from("<H", self._buffer, HEADER_SIZE + length)[0]
            if crc16_ccitt(body) != expected:
                del self._buffer[0]
                continue
            values = struct.unpack_from(PAYLOAD_FORMAT, self._buffer, HEADER_SIZE)
            decoded.append(Telemetry(*values))
            del self._buffer[:total]
        return decoded
