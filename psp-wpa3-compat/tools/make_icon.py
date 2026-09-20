#!/usr/bin/env python3
"""Write a 144x80 ICON0.PNG for the EBOOT."""
from __future__ import annotations

import struct
import zlib
from pathlib import Path


def chunk(tag: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + tag + data + struct.pack(
        ">I", zlib.crc32(tag + data) & 0xFFFFFFFF
    )


def main() -> None:
    width, height = 144, 80
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            t = x / (width - 1)
            r = int(20 + 30 * t)
            g = int(90 + 80 * t)
            b = int(140 + 70 * (1 - t))
            if 34 < y < 46 and 18 < x < 126:
                r, g, b = 240, 244, 232
            raw.extend((r, g, b))
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    out = Path(__file__).resolve().parents[1] / "assets" / "ICON0.PNG"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(png)
    print(f"wrote {out} ({out.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
