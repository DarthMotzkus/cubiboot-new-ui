#!/usr/bin/env python3
"""Generates patches/data/zbtn_tex.bin -- the Z-button pill drawn in the menu header.

64x32 GX_TF_RGB5A3, 4x4-texel tiles, big-endian, transparent outside the pill.
Pure Python (no Pillow): rendered 4x supersampled, alpha from coverage.
Rerun after tweaking and rebuild; the Makefile embeds every data/*.bin.
"""
import struct, os

W, H = 64, 32          # texture size (texels)
SS = 4                 # supersample factor
SW, SH = W * SS, H * SS

# pill geometry in supersampled pixels
PILL_X0, PILL_X1 = 4 * SS, 60 * SS
PILL_Y0, PILL_Y1 = 7 * SS, 25 * SS
RADIUS = (PILL_Y1 - PILL_Y0) / 2
RIM = 1.2 * SS         # darker outline thickness

TOP = (0x82, 0x72, 0xCD)     # gradient ends; mid-height average = #6B5CB1
BOTTOM = (0x4D, 0x42, 0x7F)
RIM_COLOR = (0x35, 0x2C, 0x63)
WHITE = (0xFF, 0xFF, 0xFF)

def pill_sdf(x, y):
    """Signed distance to the pill edge (negative = inside)."""
    cy = (PILL_Y0 + PILL_Y1) / 2
    ax0, ax1 = PILL_X0 + RADIUS, PILL_X1 - RADIUS
    px = min(max(x, ax0), ax1)
    return ((x - px) ** 2 + (y - cy) ** 2) ** 0.5 - RADIUS

def load_ipl_z():
    """The 'Z' glyph from the IPL's own ANSI font (the font the BIOS UI uses),
    pulled out of the Yay0 archive at 0x1FCF00 of a ROM dump. Returns a 24x24
    intensity map (0..255), or None when no dump is around (vector fallback)."""
    rom_path = os.path.join(os.path.dirname(__file__), "..", "..", "bios", "gc-ntsc-11.bin")
    try:
        rom = open(rom_path, "rb").read()
    except OSError:
        return None
    src = rom[0x1FCF00:]
    usize, coff, doff = struct.unpack(">III", src[4:16])
    out = bytearray(); mask = 0; mbits = 0; mpos = 16
    while len(out) < usize:
        if mbits == 0:
            mask = struct.unpack(">I", src[mpos:mpos + 4])[0]; mpos += 4; mbits = 32
        if mask & 0x80000000:
            out.append(src[doff]); doff += 1
        else:
            code = struct.unpack(">H", src[coff:coff + 2])[0]; coff += 2
            dist = (code & 0xFFF) + 1
            n = code >> 12
            if n == 0:
                n = src[doff] + 18; doff += 1
            else:
                n += 2
            for _ in range(n):
                out.append(out[-dist])
        mask <<= 1; mbits -= 1
    font = bytes(out)
    # OSFontHeader (ANSI: first_char 0x20): cells 24x24 on a 512x512 I4 sheet.
    # The sheet itself is stored 2bpp and expanded through the header's c0..c3
    # intensity table (0x00,0x55,0xAA,0xFF) -- same as OSInitFont does.
    first_char, = struct.unpack(">H", font[2:4])
    cellw, cellh = struct.unpack(">2H", font[16:20])
    col, = struct.unpack(">H", font[26:28])
    sheet_w, = struct.unpack(">H", font[30:32])
    sheet_image, = struct.unpack(">I", font[36:40])
    lut = [0x0, 0x5, 0xA, 0xF]
    exp = bytearray()
    for b in font[sheet_image:sheet_image + (sheet_w * 512 // 4)]:
        exp.append(lut[(b >> 6) & 3] << 4 | lut[(b >> 4) & 3])
        exp.append(lut[(b >> 2) & 3] << 4 | lut[b & 3])

    idx = ord("Z") - first_char
    cx, cy = (idx % col) * cellw, (idx // col) * cellh
    cell = [[0] * cellw for _ in range(cellh)]
    tiles_per_row = sheet_w // 8
    for y in range(cellh):
        for x in range(cellw):
            px, py = cx + x, cy + y
            off = ((py // 8) * tiles_per_row + (px // 8)) * 32 + (py % 8) * 4 + (px % 8) // 2
            b = exp[off]
            v = (b >> 4) if (px % 2) == 0 else (b & 0xF)
            cell[y][x] = v * 17
    # crop to the glyph's bounding box so scaling keys off the ink, not the cell
    ys = [y for y in range(cellh) if any(cell[y])]
    xs = [x for x in range(cellw) if any(cell[y][x] for y in range(cellh))]
    if not ys or not xs:
        return None
    return [row[xs[0]:xs[-1] + 1] for row in cell[ys[0]:ys[-1] + 1]]

IPL_Z = load_ipl_z()

def z_glyph_alpha(x, y):
    """Ink coverage 0..255 of the Z at supersampled (x,y)."""
    ZCX, ZCY = 32 * SS, (PILL_Y0 + PILL_Y1) / 2  # centered in the pill ink (x 4..60)
    if IPL_Z is not None:
        gh_cells = len(IPL_Z)
        gw_cells = len(IPL_Z[0])
        GH = 13 * SS  # on-texture glyph height, supersampled
        scale = GH / gh_cells
        GW = gw_cells * scale
        gx = (x - (ZCX - GW / 2)) / scale
        gy = (y - (ZCY - GH / 2)) / scale
        if 0 <= gx < gw_cells and 0 <= gy < gh_cells:
            return IPL_Z[int(gy)][int(gx)]
        return 0
    # vector fallback when no ROM dump is available
    zw, zh, t = 5.0 * SS, 11.0 * SS, 2.4 * SS
    lx, rx = ZCX - zw, ZCX + zw
    ty, by = ZCY - zh / 2, ZCY + zh / 2
    if lx <= x <= rx and ty <= y <= ty + t: return 255
    if lx <= x <= rx and by - t <= y <= by: return 255
    if ty + t <= y <= by - t:
        f = (y - (ty + t)) / max(by - t - (ty + t), 1)
        dx = rx - f * (rx - lx)
        if abs(x - dx) <= t * 0.75: return 255
    return 0

def dot(x, y):
    DCX, DCY, R = 50 * SS, (PILL_Y0 + PILL_Y1) / 2, 2.2 * SS
    return (x - DCX) ** 2 + (y - DCY) ** 2 <= R * R

# render supersampled RGBA
img = [[(0, 0, 0, 0)] * SW for _ in range(SH)]
for y in range(SH):
    for x in range(SW):
        d = pill_sdf(x + 0.5, y + 0.5)
        if d > 0.5:
            continue
        f = (y - PILL_Y0) / max(PILL_Y1 - PILL_Y0, 1)
        f = min(max(f, 0.0), 1.0)
        col = tuple(int(TOP[i] + (BOTTOM[i] - TOP[i]) * f) for i in range(3))
        if d > -RIM:
            col = RIM_COLOR
        ink = z_glyph_alpha(x + 0.5, y + 0.5)
        if dot(x + 0.5, y + 0.5):
            ink = 255
        if ink:
            col = tuple(int(col[i] + (WHITE[i] - col[i]) * ink / 255) for i in range(3))
        img[y][x] = (*col, 255)

# downsample to W x H
pix = [[(0, 0, 0, 0)] * W for _ in range(H)]
for y in range(H):
    for x in range(W):
        r = g = b = a = 0
        for sy in range(SS):
            for sx in range(SS):
                pr, pg, pb, pa = img[y * SS + sy][x * SS + sx]
                r += pr * pa; g += pg * pa; b += pb * pa; a += pa
        n = SS * SS
        if a:
            pix[y][x] = (r // a, g // a, b // a, a // n)

def rgb5a3(r, g, b, a):
    if a >= 224:
        return 0x8000 | (r >> 3) << 10 | (g >> 3) << 5 | (b >> 3)
    return (a >> 5) << 12 | (r >> 4) << 8 | (g >> 4) << 4 | (b >> 4)

out = bytearray()
for ty in range(0, H, 4):
    for tx in range(0, W, 4):
        for y in range(ty, ty + 4):
            for x in range(tx, tx + 4):
                out += struct.pack(">H", rgb5a3(*pix[y][x]))

dst = os.path.join(os.path.dirname(__file__), "..", "..", "patches", "data", "zbtn_tex.bin")
open(os.path.abspath(dst), "wb").write(out)
print(f"wrote {os.path.abspath(dst)}: {len(out)} bytes ({W}x{H} RGB5A3)")
