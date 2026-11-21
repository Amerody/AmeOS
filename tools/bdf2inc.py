#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later

"""bdf2inc.py: convert a fixed-width BDF bitmap font into C array initializers.

Emits one designated-initializer entry per glyph, in the same format as a
hand-written table:

    [65] = { 0x00, 0x00, 0x18, ... },  // U+0041 (A)

so the output can be #included inside the table definition:

    static const uint8_t font_data[256][FONT_HEIGHT] = {
    #include "terminus_16.inc"
    };

Only codes 0..255 are emitted (the table is indexed by an unsigned char).
Glyphs missing from the BDF stay zero. The cell width must be 8 pixels or less
(one byte per row); the cell height is whatever the font's bounding box says.

The output starts with a license header for the font, since the generated data
is a converted copy of the font and stays under the font's own license. The
copyright statement is read from the license file (by default OFL.TXT next to
the .bdf), falling back to the BDF's COPYRIGHT property.

Usage:
    py tools\\bdf2inc.py terminus/ter-u16n.bdf include/gfx/terminus_16.inc

Options:
    --license-file PATH   license text to read the copyright from and point to
                          (default: OFL.TXT in the same folder as the .bdf)
    --spdx ID             SPDX identifier for the font's license (default: OFL-1.1)
"""

import argparse
import sys
from pathlib import Path

# Repository root: this script lives in <root>/tools/.
REPO_ROOT = Path(__file__).resolve().parent.parent


def repo_path(path):
    """Return path relative to the repo root with forward slashes, if possible."""
    p = Path(path).resolve()
    try:
        return p.relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return p.name


def read_copyright(license_file):
    """Return the copyright lines at the top of a license file such as OFL.TXT.

    OFL.TXT starts with the copyright statement (possibly spanning several
    lines, e.g. the Reserved Font Name), followed by a blank line.
    """
    lines = []
    with open(license_file, encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.strip()
            if not line:
                if lines:
                    break
                continue
            if not lines and "copyright" not in line.lower():
                return []                  # file doesn't start with a copyright statement
            lines.append(line)
    return lines


def parse_bdf(path):
    """Return {"bbox": (w, h, xoff, yoff), "glyphs": {code: glyph}}."""
    font = {"bbox": None, "glyphs": {}, "copyright": None}
    glyph = None
    in_bitmap = False

    with open(path, encoding="latin-1") as f:
        for raw in f:
            line = raw.strip()

            if in_bitmap:
                if line == "ENDCHAR":
                    in_bitmap = False
                    if glyph["enc"] >= 0 and glyph["bbx"] is not None:
                        font["glyphs"][glyph["enc"]] = glyph
                    glyph = None
                elif line:
                    glyph["rows"].append(line)
                continue

            parts = line.split()
            if not parts:
                continue
            key = parts[0]

            if key == "FONTBOUNDINGBOX":
                font["bbox"] = tuple(int(v) for v in parts[1:5])
            elif key == "COPYRIGHT" and glyph is None:
                font["copyright"] = line[len("COPYRIGHT"):].strip().strip('"')
            elif key == "STARTCHAR":
                glyph = {"enc": -1, "bbx": None, "rows": []}
            elif key == "ENCODING" and glyph is not None:
                glyph["enc"] = int(parts[1])
            elif key == "BBX" and glyph is not None:
                glyph["bbx"] = tuple(int(v) for v in parts[1:5])
            elif key == "BITMAP" and glyph is not None:
                in_bitmap = True

    if font["bbox"] is None:
        sys.exit("error: no FONTBOUNDINGBOX found, is this a BDF file?")
    return font


def glyph_to_rows(g, cell_w, cell_h, cell_xoff, cell_yoff):
    """Place a glyph's bitmap into the font cell; return cell_h row bytes.

    BDF's y axis points up and each glyph has its own bounding box (BBX), so
    the glyph is positioned relative to the cell's bounding box instead of
    assuming it fills the cell.
    """
    gw, gh, gx, gy = g["bbx"]
    rows = [0] * cell_h

    cell_top = cell_h + cell_yoff          # y of the cell's top edge
    first_row = cell_top - (gy + gh)       # cell row of the glyph's first bitmap row
    dx = gx - cell_xoff                    # cell column of the glyph's leftmost pixel
    shift = 8 - gw - dx                    # moves the glyph's pixels into an 8-bit row

    for i, hexrow in enumerate(g["rows"][:gh]):
        r = first_row + i
        if not 0 <= r < cell_h:
            continue                       # outside the cell: clipped
        nbits = len(hexrow) * 4
        pixels = int(hexrow, 16) >> (nbits - gw)   # keep the gw real pixels
        byte = (pixels << shift) if shift >= 0 else (pixels >> -shift)
        rows[r] = byte & 0xFF

    return rows


def main():
    ap = argparse.ArgumentParser(description="Convert a fixed-width BDF font to C initializers.")
    ap.add_argument("bdf", help="input .bdf file (for example Terminus ter-u16n.bdf)")
    ap.add_argument("out", help="output file to #include inside the table definition")
    ap.add_argument("--license-file", help="font license text (default: OFL.TXT next to the .bdf)")
    ap.add_argument("--spdx", default="OFL-1.1", help="SPDX identifier of the font's license (default: OFL-1.1)")
    args = ap.parse_args()

    font = parse_bdf(args.bdf)
    cell_w, cell_h, cell_xoff, cell_yoff = font["bbox"]
    if cell_w > 8:
        sys.exit(f"error: cell is {cell_w} pixels wide; this script supports 8 or less")

    license_file = Path(args.license_file) if args.license_file else Path(args.bdf).parent / "OFL.TXT"
    if not license_file.is_file():
        sys.exit(f"error: license file {license_file} not found; pass --license-file")

    copyright_lines = read_copyright(license_file)
    if not copyright_lines and font["copyright"]:
        copyright_lines = [font["copyright"]]
    if not copyright_lines:
        sys.exit(f"error: no copyright statement found in {license_file} or in the BDF")

    count = 0
    with open(args.out, "w", encoding="utf-8", newline="\n") as out:
        out.write(f"// SPDX-License-Identifier: {args.spdx}\n")
        for line in copyright_lines:
            out.write(f"// {line}\n")
        out.write(f"// License text: {repo_path(license_file)}\n")
        out.write("//\n")
        out.write(f"// Generated from {repo_path(args.bdf)} by tools/bdf2inc.py. Do not edit by hand.\n")
        out.write(f"// Cell: {cell_w}x{cell_h} pixels, one byte per row, bit 7 = leftmost pixel.\n")
        for code in sorted(c for c in font["glyphs"] if c <= 255):
            rows = glyph_to_rows(font["glyphs"][code], cell_w, cell_h, cell_xoff, cell_yoff)
            body = ", ".join(f"0x{b:02X}" for b in rows)
            label = f" ({chr(code)})" if 33 <= code <= 126 else ""
            out.write(f"[{code}] = {{ {body} }},  // U+{code:04X}{label}\n")
            count += 1

    print(f"{count} glyphs written to {args.out}")
    print(f"cell: {cell_w}x{cell_h}  ->  set FONT_WIDTH {cell_w}, FONT_HEIGHT {cell_h}")


if __name__ == "__main__":
    main()