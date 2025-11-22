import struct
from dataclasses import dataclass
from typing import Optional, Tuple

from schemas.gpsdo.Message import Message
from schemas.gpsdo.Payload import Payload
from schemas.gpsdo.Status import Status
from schemas.gpsdo.kf_Debug import kf_debug

FLATBUF_MAGIC = 0xB00B
MAX_MESSAGE_SIZE = 1024
HEADER_FMT = "<HHH"  # little-endian: magic, msg_id, len
HEADER_SIZE = struct.calcsize(HEADER_FMT)


@dataclass
class ParsedMessage:
    msg_id: int
    payload_type: int
    timestamp_s: float
    payload: object


class FlatbufferStreamReader:
    def __init__(self, ser, log, max_message_size: int = MAX_MESSAGE_SIZE):
        self.ser = ser
        self.log = log
        self.max_message_size = max_message_size

    def read_next(self) -> Optional[ParsedMessage]:
        frame = self._read_frame()
        if frame is None:
            return None

        msg_id, payload = frame
        return self._decode_payload(msg_id, payload)

    def _read_frame(self) -> Optional[Tuple[int, bytes]]:
        header = self.ser.read(HEADER_SIZE)
        if len(header) < HEADER_SIZE:
            return None

        magic, msg_id, msg_len = struct.unpack(HEADER_FMT, header)

        if magic != FLATBUF_MAGIC:
            self.log.warning(f"Bad magic 0x{magic:04X}, searching for next valid frame...")
            magic = self._resync()
            if magic != FLATBUF_MAGIC:
                return None
            rest_header = self.ser.read(HEADER_SIZE - 2)
            if len(rest_header) < (HEADER_SIZE - 2):
                return None
            header = struct.pack("<H", magic) + rest_header
            magic, msg_id, msg_len = struct.unpack(HEADER_FMT, header)

        if msg_len <= 0 or msg_len > self.max_message_size:
            self.log.warning(f"Invalid message length {msg_len}, skipping")
            return None

        payload = self.ser.read(msg_len)
        if len(payload) < msg_len:
            self.log.warning("Incomplete message, skipping")
            return None

        return msg_id, payload

    def _decode_payload(self, msg_id: int, payload: bytes) -> Optional[ParsedMessage]:
        try:
            message = Message.GetRootAs(payload, 0)
        except Exception as exc:
            self.log.warning(f"Failed to parse Message envelope (msg_id={msg_id}): {exc}")
            return None

        payload_type = message.PayloadType()
        table = message.Payload()
        if table is None:
            self.log.warning(f"Message has no payload table (msg_id={msg_id}, payload_type={payload_type})")
            return None

        if payload_type == Payload.Status:
            obj = Status()
            obj.Init(table.Bytes, table.Pos)
        elif payload_type == Payload.kf_debug:
            obj = kf_debug()
            obj.Init(table.Bytes, table.Pos)
        else:
            self.log.info(f"Unknown payload_type={payload_type}, msg_id={msg_id}, skipping")
            return None

        if msg_id != 0 and msg_id != payload_type:
            self.log.debug(f"Header msg_id {msg_id} != payload_type {payload_type}")

        self.log.debug(
            f"Decoded message msg_id={msg_id} payload_type={payload_type} timestamp_s={message.TimestampS()}"
        )

        return ParsedMessage(
            msg_id=msg_id,
            payload_type=payload_type,
            timestamp_s=message.TimestampS(),
            payload=obj,
        )

    def _resync(self, log=None) -> int:
        """Scan stream for next magic word (2-byte sliding window)"""
        # Allow overriding log to reuse existing warning/info style while resyncing
        logger = log or self.log
        window = bytearray(2)
        idx = 0
        logger.debug("Resyncing...")
        while True:
            b = self.ser.read(1)
            if not b:
                continue
            window[idx] = b[0]
            idx = 1 - idx  # toggle index to maintain sliding 2-byte window
            val = window[0] | (window[1] << 8)
            if val == FLATBUF_MAGIC:
                logger.info("Resynced to magic word")
                return val
