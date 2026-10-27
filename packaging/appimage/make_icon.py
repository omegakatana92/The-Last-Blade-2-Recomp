#!/usr/bin/env python3
"""Draw the project's original 256x256 icon (no game artwork).

The icon is generated from primitives so the repository contains no
copyrighted art: a midnight-to-crimson field, a katana-like diagonal, and
the letters LB2 in a small block font.
"""
import struct
import zlib
from pathlib import Path

W = H = 256
px = [[(0, 0, 0, 255) for _ in range(W)] for _ in range(H)]

# Background: vertical gradient, midnight blue at the top to deep crimson.
for y in range(H):
    t = y / (H - 1)
    r = int(10 + 105 * t)
    g = int(14 + 18 * t)
    b = int(28 + 20 * (1 - t))
    for x in range(W):
        px[y][x] = (r, g, b, 255)

# A faint diagonal "blade" band behind the letters.
for y in range(H):
    for x in range(W):
        d = abs((x - 40) - (y - 20))
        if d < 7:
            r, g, b, a = px[y][x]
            k = 0.35 * (1 - d / 7)
            px[y][x] = (int(r + (235 - r) * k), int(g + (238 - g) * k),
                        int(b + (245 - b) * k), 255)

# Gold border.
for i in range(3):
    for x in range(W):
        px[i][x] = px[H - 1 - i][x] = (214, 170, 70, 255)
    for y in range(H):
        px[y][i] = px[y][W - 1 - i] = (214, 170, 70, 255)

# 5x7 block font.
FONT = {
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "B": ["11110", "10001", "10001", "11110", "10001", "10001", "11110"],
    "2": ["01110", "10001", "00001", "00010", "00100", "01000", "11111"],
}
TEXT = "LB2"
scale = 14
glyph_w = 5 * scale
gap = 12
total_w = glyph_w * len(TEXT) + gap * (len(TEXT) - 1)
x0 = (W - total_w) // 2
y0 = 96

# Drop shadow.
for ox, oy in ((4, 4),):
    cx = x0 + ox
    for ch in TEXT:
        for gy, row in enumerate(FONT[ch]):
            for gx, bit in enumerate(row):
                if bit == "1":
                    for yy in range(scale):
                        for xx in range(scale):
                            X, Y = cx + gx * scale + xx, y0 + oy + gy * scale + yy
                            if 0 <= X < W and 0 <= Y < H:
                                px[Y][X] = (20, 8, 10, 255)
        cx += glyph_w + gap

# Letters.
cx = x0
for ch in TEXT:
    for gy, row in enumerate(FONT[ch]):
        for gx, bit in enumerate(row):
            if bit == "1":
                for yy in range(scale):
                    for xx in range(scale):
                        X, Y = cx + gx * scale + xx, y0 + gy * scale + yy
                        if 0 <= X < W and 0 <= Y < H:
                            px[Y][X] = (250, 245, 225, 255)
    cx += glyph_w + gap

# A small red seal mark under the letters.
for y in range(206, 234):
    for x in range(112, 144):
        px[y][x] = (170, 28, 36, 255)
for y in range(211, 229):
    for x in range(118, 138):
        px[y][x] = (232, 200, 120, 255)
for y in range(215, 225):
    for x in range(122, 134):
        px[y][x] = (170, 28, 36, 255)


def chunk(tag, data):
    return (struct.pack(">I", len(data)) + tag + data +
            struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))


raw = bytearray()
for y in range(H):
    raw.append(0)
    for x in range(W):
        raw.extend(px[y][x])

png = (b"\x89PNG\r\n\x1a\n" +
       chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0)) +
       chunk(b"IDAT", zlib.compress(bytes(raw), 9)) +
       chunk(b"IEND", b""))

out = Path(__file__).with_name("lastblade2.png")
out.write_bytes(png)
print("wrote", out)
