"""Convert ordinary images into GameCube banner artwork.

Two outputs, picked from the menu at startup:

  1. A 16-bit BMP in the exact shape GCRebuilder wants, to import into an existing
     opening.bnr.
  2. A complete opening.bnr, for a homebrew app folder that ships its own banner
     next to default.dol.

Either way the source image can be any size and any of the common formats; it is
fitted into the 96x32 the console expects.
"""

import argparse
import os
import struct
import sys

from PIL import Image, ImageColor

# The console's banner size. Everything is fitted into this.
WIDTH, HEIGHT = 96, 32

# 96x32 is a 3:1 slot, and almost no source image is 3:1. How to reconcile the two:
#
#   contain  scale until the whole image fits, pad the leftover with --pad. Nothing is
#            lost, nothing is distorted; a squarish source ends up small and surrounded.
#   cover    scale until the slot is full and crop the overflow from the centre. Fills
#            the banner, at the cost of the edges.
#   stretch  force it to 96x32. Distorts, and is what every version before this did.
FIT_MODES = ("contain", "cover", "stretch")
DEFAULT_FIT = "contain"
DEFAULT_PAD = (0, 0, 0, 0)

VALID_EXTENSIONS = ('.png', '.jpg', '.jpeg', '.webp', '.bmp', '.gif', '.tga')

# --- opening.bnr layout -------------------------------------------------------------
# magic[4] + padding[0x1C] + pixelData[0x1800] + BNRDesc[n]
#
# BNRDesc is gameName[0x20] company[0x20] fullGameName[0x40] fullCompany[0x40]
# description[0x80] = 0x140 bytes. 'BNR1' carries one of them, 'BNR2' carries six (one
# per language). BNR1 is what this writes: a homebrew app has a single set of strings,
# and every reader handles it.
BNR_MAGIC = b'BNR1'
BNR_HEADER_SIZE = 0x20
BNR_PIXELDATA_SIZE = WIDTH * HEIGHT * 2      # 0x1800
DESC_FIELDS = (0x20, 0x20, 0x40, 0x40, 0x80)  # in write order


def to_rgb5a3(r, g, b, a):
    """Pack one pixel the way the GPU reads RGB5A3.

    Bit 15 selects the interpretation: set means opaque RGB555, clear means RGB444
    with 3 bits of alpha. Fully opaque pixels therefore keep a bit more colour
    precision, which is what almost every banner wants.
    """
    if a >= 0xF8:
        return 0x8000 | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3)

    return ((a >> 5) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4)


def encode_banner_pixels(img):
    """Encode a 96x32 image as tiled big-endian RGB5A3.

    GC textures are not stored as scanlines. A 16-bit texture is cut into 4x4 pixel
    tiles, each tile written out row by row, and the tiles themselves written left to
    right, top to bottom. Writing plain scanlines instead produces a banner that looks
    shredded into diagonal blocks -- it is the single easiest thing to get wrong here.
    """
    pixels = img.load()
    out = bytearray()

    for tile_y in range(0, HEIGHT, 4):
        for tile_x in range(0, WIDTH, 4):
            for y in range(tile_y, tile_y + 4):
                for x in range(tile_x, tile_x + 4):
                    out += struct.pack(">H", to_rgb5a3(*pixels[x, y]))

    return bytes(out)


def pack_text(value, size):
    """Fit a string into a fixed-size field, always NUL terminated.

    The reader treats these as C strings, so a value that filled the field exactly
    would run on into the next one. Reserve the last byte.
    """
    encoded = value.encode('latin-1', errors='replace')[:size - 1]
    return encoded + b'\x00' * (size - len(encoded))


def fit_image(img, fit, pad):
    """Bring any image down to 96x32, honouring the aspect ratio unless told not to."""
    if img.size == (WIDTH, HEIGHT):
        return img

    if fit == "stretch":
        return img.resize((WIDTH, HEIGHT), Image.Resampling.LANCZOS)

    src_w, src_h = img.size
    pick = max if fit == "cover" else min
    scale = pick(WIDTH / src_w, HEIGHT / src_h)

    # Round rather than truncate, or a source that is already 3:1 can land on 95 wide and
    # pick up a one-pixel bar it does not deserve.
    new_w = max(1, round(src_w * scale))
    new_h = max(1, round(src_h * scale))
    scaled = img.resize((new_w, new_h), Image.Resampling.LANCZOS)

    if fit == "cover":
        left = (new_w - WIDTH) // 2
        top = (new_h - HEIGHT) // 2
        return scaled.crop((left, top, left + WIDTH, top + HEIGHT))

    canvas = Image.new("RGBA", (WIDTH, HEIGHT), pad)
    # No mask: copy the source alpha in as-is instead of compositing it onto the padding,
    # so a transparent pad stays transparent in the opening.bnr.
    canvas.paste(scaled, ((WIDTH - new_w) // 2, (HEIGHT - new_h) // 2))
    return canvas


def load_image(input_path, fit, pad):
    try:
        img = Image.open(input_path).convert("RGBA")
    except Exception as exc:
        print(f"[-] Could not open {os.path.basename(input_path)}: {exc}")
        return None

    return fit_image(img, fit, pad)


def parse_pad(value):
    """Accept anything Pillow names a colour ('black', '#204080', 'rgba(0,0,0,0)')."""
    if value is None:
        return DEFAULT_PAD

    try:
        rgba = ImageColor.getrgb(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(str(exc)) from exc

    return rgba if len(rgba) == 4 else rgba + (255,)


def convert_to_gcrebuilder_bmp(input_path, output_path, fit, pad):
    img = load_image(input_path, fit, pad)
    if img is None:
        return False

    pixels = list(img.getdata())
    pixel_bytes = bytearray()

    # BMP stores its rows bottom to top.
    for y in reversed(range(HEIGHT)):
        for x in range(WIDTH):
            r, g, b, a = pixels[y * WIDTH + x]

            r5 = (r >> 3) & 0x1F
            g5 = (g >> 3) & 0x1F
            b5 = (b >> 3) & 0x1F

            # Do NOT set the alpha bit (bit 15) -- leave it at 0.
            # GCRebuilder hardcodes "ignoreBannerAlpha" to TRUE and, on import, computes
            # (low_byte | 0x8000) + (high_byte << 8). If bit 15 is already 1 the addition
            # carries into bit 16 and CLEARS bit 15, so the pixel gets read as A3R4G4B4
            # and comes out as noise. With bit 15 at 0, GCRebuilder sets the opaque bit
            # itself, correctly.
            packed_pixel = (r5 << 10) | (g5 << 5) | b5
            pixel_bytes.extend(struct.pack("<H", packed_pixel))

    # The BMP header in the EXACT shape GCRebuilder accepts. NOT the V3 header with
    # BI_BITFIELDS: GCRebuilder requires the classic 40-byte BITMAPINFOHEADER with
    # biCompression = 0 (BI_RGB) and no colour masks. Under BI_RGB, 16bpp already means
    # X1R5G5B5, which GCRebuilder reads as A1 R5 G5 B5 (top bit = alpha).
    # Reference header, as exported by GCRebuilder itself:
    #   42 4d 36 18 00 00 00 00 00 00 36 00 00 00 28 00 00 00 60 00 ...
    pixel_data_size = len(pixel_bytes)
    pixel_offset = 14 + 40                       # 54 (0x36)
    total_file_size = pixel_offset + pixel_data_size

    bmp_file_header = struct.pack("<2sIHHI", b'BM', total_file_size, 0, 0, pixel_offset)

    bmp_info_header = struct.pack(
        "<IiiHHIIiiII",
        40,          # biSize = 40 (BITMAPINFOHEADER)
        WIDTH,       # biWidth = 96
        HEIGHT,      # biHeight = 32
        1,           # biPlanes = 1
        16,          # biBitCount = 16
        0,           # biCompression = 0 (BI_RGB)  <-- NOT 3 (BITFIELDS)
        0,           # biSizeImage (0 is valid for BI_RGB)
        0, 0,        # biX/YPelsPerMeter
        0, 0         # biClrUsed / biClrImportant
    )

    with open(output_path, "wb") as handle:
        handle.write(bmp_file_header)
        handle.write(bmp_info_header)
        handle.write(pixel_bytes)

    return True


def convert_to_opening_bnr(input_path, output_path, title, author, description, fit, pad):
    img = load_image(input_path, fit, pad)
    if img is None:
        return False

    blob = bytearray()
    blob += BNR_MAGIC
    blob += b'\x00' * (BNR_HEADER_SIZE - len(BNR_MAGIC))
    blob += encode_banner_pixels(img)

    # Short and long variants of the same two strings: readers pick whichever fits the
    # space they have. A homebrew app has no reason to differ between them.
    for value, size in zip((title, author, title, author, description), DESC_FIELDS):
        blob += pack_text(value, size)

    expected = BNR_HEADER_SIZE + BNR_PIXELDATA_SIZE + sum(DESC_FIELDS)
    assert len(blob) == expected, f"built {len(blob)} bytes, expected {expected}"

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "wb") as handle:
        handle.write(blob)

    return True


def ask(prompt, default):
    answer = input(f"{prompt} [{default}]: ").strip()
    return answer if answer else default


def choose_mode():
    print("What do you want to build?")
    print("  [1] BMP for GCRebuilder  - to import into an existing opening.bnr")
    print("  [2] opening.bnr          - a complete banner file, for an app folder")
    print()

    while True:
        choice = input("Choice [1/2]: ").strip()
        if choice == "1":
            return "bmp"
        if choice == "2":
            return "bnr"
        print("Type 1 or 2.")


def choose_fit(mode):
    padding = "transparent" if mode == "bnr" else "black"

    print("The banner slot is 96x32 (3:1). How should an image of another shape fit?")
    print(f"  [1] contain - whole image, {padding} bars where it does not reach")
    print("  [2] cover   - fill the banner, crop what overflows")
    print("  [3] stretch - squash it to 3:1 (distorts; the old behaviour)")
    print()

    while True:
        choice = input("Choice [1/2/3]: ").strip() or "1"
        if choice in ("1", "2", "3"):
            return FIT_MODES[int(choice) - 1]
        print("Type 1, 2 or 3.")


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--mode", choices=("bmp", "bnr"),
                        help="skip the menu and build this")
    parser.add_argument("--title", help="banner title (opening.bnr only)")
    parser.add_argument("--author", help="banner author or company (opening.bnr only)")
    parser.add_argument("--description", help="banner description (opening.bnr only)")
    parser.add_argument("--fit", choices=FIT_MODES,
                        help=f"how to reconcile the aspect ratio (default: {DEFAULT_FIT})")
    parser.add_argument("--pad", type=parse_pad, default=DEFAULT_PAD,
                        help="colour of the bars left by --fit contain (default: "
                             "transparent, which the GCRebuilder BMP renders as black)")
    args = parser.parse_args()

    current_dir = os.path.dirname(os.path.abspath(__file__)) if __file__ else os.getcwd()
    output_dir = os.path.join(current_dir, "output")

    images = sorted(name for name in os.listdir(current_dir)
                    if name.lower().endswith(VALID_EXTENSIONS))

    if not images:
        print(f"[-] No images found next to the script. Supported: {', '.join(VALID_EXTENSIONS)}")
        return 1

    print(f"[*] {len(images)} image(s) found.\n")
    mode = args.mode or choose_mode()
    print()

    # --mode on its own is a scripted run, so don't stop it to ask about the fit.
    fit = args.fit or (DEFAULT_FIT if args.mode else choose_fit(mode))
    print()

    os.makedirs(output_dir, exist_ok=True)
    converted = 0

    for filename in images:
        input_path = os.path.join(current_dir, filename)
        stem = os.path.splitext(filename)[0]

        if mode == "bmp":
            output_path = os.path.join(output_dir, f"{stem}.bmp")
            print(f" -> {filename}")
            if convert_to_gcrebuilder_bmp(input_path, output_path, fit, args.pad):
                converted += 1
            continue

        # Each banner gets its own folder, matching the layout an app is copied in as:
        # drop default.dol beside it and the folder is ready for the SD card.
        print(f" -> {filename}")
        # `is not None` rather than a truthiness check, so --description "" can actually
        # mean empty instead of falling through to the prompt.
        title = args.title if args.title is not None else ask("    Title", stem)
        author = args.author if args.author is not None else ask("    Author", "Unknown")
        # Not defaulted to the title. The menu prints the description on its own line under
        # the name, so accepting the default put the same text on screen twice.
        description = (args.description if args.description is not None
                       else ask("    Description (optional)", ""))

        output_path = os.path.join(output_dir, stem, "opening.bnr")
        if convert_to_opening_bnr(input_path, output_path, title, author, description,
                                  fit, args.pad):
            converted += 1
        print()

    print(f"\n[+] Done. {converted} of {len(images)} written to 'output'.")
    if mode == "bnr" and converted:
        print("    Each folder needs a default.dol next to its opening.bnr to be")
        print("    launchable as an app.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
