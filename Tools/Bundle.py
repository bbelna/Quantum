#!/usr/bin/env python3
"""
INIT.BND bundler
- Reads a JSON manifest describing bundle entries.
- Emits a binary bundle with the documented header/table format.
"""

import argparse
import json
import os
import struct
import sys


HEADER_FMT = "<8sHHI8s"  # magic, version, entryCount, tableOffset, reserved
ENTRY_FMT = "<32sBB2sIII"  # name[32], type, flags, reserved[2], offset, size, checksum
ALIGN = 4096
MAGIC = b"INITBND\x00"
VERSION = 1


TYPE_MAP = {
    "init": 1,
    "driver": 2,
    "service": 3,
}


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def load_entries(manifest_path: str, base_path: str):
    with open(manifest_path, "r", encoding="utf-8") as f:
        manifest = json.load(f)

    entries = []
    for item in manifest:
        name = item.get("name")
        path = item.get("path")
        type_str = (item.get("type") or "").lower()
        required = bool(item.get("required"))

        if not name or not path:
            raise ValueError("Manifest entry missing 'name' or 'path'")

        name_bytes = name.encode("ascii", errors="strict")
        if len(name_bytes) > 31:
            raise ValueError(f"Entry name too long (max 31 bytes ASCII): {name}")

        entry_path = os.path.join(base_path, path)
        if not os.path.isfile(entry_path):
            raise FileNotFoundError(f"Entry file not found: {entry_path}")

        with open(entry_path, "rb") as pf:
            payload = pf.read()

        type_byte = TYPE_MAP.get(type_str, 0)
        flags = 0x01 if required else 0x00

        entries.append({
            "name": name_bytes,
            "type": type_byte,
            "flags": flags,
            "payload": payload,
        })

    return entries


def build_bundle(entries, output_path: str):
    entry_count = len(entries)
    header_size = struct.calcsize(HEADER_FMT)
    entry_size = struct.calcsize(ENTRY_FMT)
    table_size = entry_size * entry_count
    payload_start = align_up(header_size + table_size, ALIGN)

    # assign offsets and sizes
    offset = payload_start
    for e in entries:
        e["offset"] = offset
        e["size"] = len(e["payload"])
        e["checksum"] = 0  # reserved for future
        offset = align_up(offset + e["size"], ALIGN)

    with open(output_path, "wb") as out:
        # header
        out.write(struct.pack(
            HEADER_FMT,
            MAGIC,
            VERSION,
            entry_count,
            header_size,  # tableOffset is immediately after header
            b"\x00" * 8,
        ))

        # table
        for e in entries:
            name_field = e["name"].ljust(32, b"\x00")
            out.write(struct.pack(
                ENTRY_FMT,
                name_field,
                e["type"],
                e["flags"],
                b"\x00\x00",
                e["offset"],
                e["size"],
                e["checksum"],
            ))

        # pad to payload start
        current = out.tell()
        if current < payload_start:
            out.write(b"\x00" * (payload_start - current))

        # payloads with alignment
        for e in entries:
            out.write(e["payload"])
            current = out.tell()
            next_off = align_up(current, ALIGN)
            if next_off > current:
                out.write(b"\x00" * (next_off - current))


def main():
    parser = argparse.ArgumentParser(description="Build INIT.BND bundle")
    parser.add_argument("--manifest", "-m", required=True, help="Path to manifest JSON")
    parser.add_argument("--output", "-o", required=True, help="Path to output INIT.BND")
    parser.add_argument("--base", "-b", default=".", help="Base path for entry files")
    args = parser.parse_args()

    entries = load_entries(args.manifest, args.base)
    if not entries:
        print("No entries in manifest", file=sys.stderr)
        sys.exit(1)

    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    build_bundle(entries, args.output)
    print(f"INIT.BND written to {args.output} ({len(entries)} entries)")


if __name__ == "__main__":
    main()
