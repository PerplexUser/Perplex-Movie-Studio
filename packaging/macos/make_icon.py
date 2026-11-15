#!/usr/bin/env python3
"""Wrap the existing icon's PNG frames in ICNS without changing their pixels."""
from pathlib import Path
import struct

root = Path(__file__).resolve().parents[2]
ico = (root / "assets/perplex.ico").read_bytes()
_, kind, count = struct.unpack_from("<HHH", ico)
assert kind == 1
tags = {16: [b"icp4"], 32: [b"icp5", b"ic11"], 64: [b"icp6", b"ic12"],
        128: [b"ic07"], 256: [b"ic08", b"ic13"]}
chunks = []
for i in range(count):
    width, height, _, _, _, _, length, offset = struct.unpack_from("<BBBBHHII", ico, 6 + 16 * i)
    width, height = width or 256, height or 256
    data = ico[offset:offset + length]
    if width != height or width not in tags:
        continue
    assert data.startswith(b"\x89PNG\r\n\x1a\n")
    for tag in tags[width]:
        chunks.append(tag + struct.pack(">I", len(data) + 8) + data)
assert chunks
body = b"".join(chunks)
(root / "assets/perplex.icns").write_bytes(b"icns" + struct.pack(">I", len(body) + 8) + body)
