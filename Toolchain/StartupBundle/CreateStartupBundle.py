#!/usr/bin/env python3

import argparse
import json
import os
import struct
import sys

HEADER_FMT = "<8sHHI8s"  # magic, version, entryCount, tableOffset, reserved
ENTRY_FMT = "<32sBBBIII"  # name[32], type, flags, format,
                           # offset, size, checksum
ALIGN = 4096
MAGIC = b"STARTUP\x00"
VERSION = 1

TYPE_MAP = {
  "system": 1,
  "driver": 2,
  "application": 3,
  "filesystem": 4
}

FORMAT_MAP = {
  "binary": 1,
  "elf32": 2,
  "disk": 3
}

def align_up(value: int, alignment: int) -> int:
  return (value + alignment - 1) & ~(alignment - 1)

def load_entries(manifest_path: str, base_path: str) -> list:
  with open(manifest_path, "r", encoding="utf-8") as f:
    manifest = json.load(f)

  entries = []

  for item in manifest:
    name = item.get("name")
    path = item.get("path")
    type_str = (item.get("type") or "").lower()
    format_str = item.get("format")
    required = bool(item.get("required"))
    is_startup = bool(item.get("startup"))

    if not name:
      raise ValueError("Manifest entry missing 'name'")

    name_bytes = name.encode("ascii", errors="strict")

    if len(name_bytes) > 31:
      raise ValueError(f"Entry name too long (max 31 bytes ASCII): {name}")

    type_byte = TYPE_MAP.get(type_str, 0)
    flags = 0x01 if required else 0x00
    format_value = 0

    if isinstance(format_str, str):
      format_value = FORMAT_MAP.get(format_str.lower(), 0)
    elif isinstance(format_str, int):
      format_value = format_str

    if format_str == "disk":
      disk_path = item.get("diskPath")

      if not disk_path:
        raise ValueError(f"Disk entry '{name}' missing 'diskPath'")

      payload = disk_path.encode("ascii", errors="strict") + b"\x00"
    else:
      if not path:
        raise ValueError(f"Manifest entry '{name}' missing 'path'")

      entry_path = os.path.join(base_path, path)

      if not os.path.isfile(entry_path):
        raise FileNotFoundError(f"Entry file not found: {entry_path}")

      with open(entry_path, "rb") as pf:
        payload = pf.read()

    entries.append({
      "name": name_bytes,
      "nameStr": name,
      "type": type_byte,
      "flags": flags,
      "format": format_value,
      "payload": payload,
      "startup": is_startup,
    })

  return entries

def select_startup_entry(entries: list) -> dict:
  explicit = [e for e in entries if e.get("startup")]

  if len(explicit) > 1:
    raise ValueError("Multiple manifest entries marked as startup")

  if explicit:
    return explicit[0]

  for entry in entries:
    if entry.get("nameStr", "").lower() == "startupserver":
      return entry

  return entries[0]

def write_bundle(entries, out, base_offset: int = 0):
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

  if base_offset:
    out.seek(base_offset)

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
      e["format"],
      e["offset"],
      e["size"],
      e["checksum"],
    ))

  # pad to payload start
  current = out.tell() - base_offset

  if current < payload_start:
    out.write(b"\x00" * (payload_start - current))

  # payloads with alignment
  for e in entries:
    out.write(e["payload"])

    current = out.tell() - base_offset
    next_off = align_up(current, ALIGN)

    if next_off > current:
      out.write(b"\x00" * (next_off - current))

def build_bundle(entries, output_path: str):
  with open(output_path, "wb") as out:
    write_bundle(entries, out)

def resolve_manifest(base_path: str, platform: str, arch: str, media: str) -> str:
  """Derive the manifest path from platform/arch/media.

  Convention:
    Toolchain/Platform/{PLATFORM}/StartupManifests/
      StartupManifest-{PLATFORM}-{ARCH}-{MEDIA}.json
  """
  manifest_path = os.path.join(
    base_path, "Toolchain", "Platform", platform,
    "StartupManifests",
    f"StartupManifest-{platform}-{arch}-{media}.json",
  )

  if not os.path.isfile(manifest_path):
    raise FileNotFoundError(
      f"No startup manifest for {platform}/{arch}/{media}: {manifest_path}"
    )

  return manifest_path


def main():
  parser = argparse.ArgumentParser(description="Build Startup")

  parser.add_argument(
    "--manifest",
    "-m",
    default=None,
    help="Explicit path to manifest JSON (overrides --platform/--arch/--media)"
  )
  parser.add_argument(
    "--platform",
    default=None,
    help="Target platform (e.g. PC)"
  )
  parser.add_argument(
    "--arch",
    default=None,
    help="Target architecture (e.g. IA32)"
  )
  parser.add_argument(
    "--media",
    default=None,
    help="Boot medium (e.g. Floppy)"
  )
  parser.add_argument(
    "--output",
    "-o",
    required=True,
    help="Output startup file path"
  )
  parser.add_argument(
    "--base",
    "-b",
    default=".",
    help="Base path for manifest entries (and manifest lookup)"
  )

  args = parser.parse_args()

  if args.manifest:
    manifest_path = args.manifest
  elif args.platform and args.arch and args.media:
    manifest_path = resolve_manifest(
      args.base, args.platform, args.arch, args.media
    )
  else:
    parser.error(
      "Either --manifest or all of --platform/--arch/--media are required"
    )

  entries = load_entries(manifest_path, args.base)

  if not entries:
    print("No entries in manifest", file=sys.stderr)
    sys.exit(1)

  startup_entry = select_startup_entry(entries)
  bundle_entries = [e for e in entries if e is not startup_entry]

  os.makedirs(os.path.dirname(args.output), exist_ok=True)

  with open(args.output, "wb") as out:
    startup_payload = startup_entry["payload"]
    out.write(startup_payload)

    bundle_offset = align_up(len(startup_payload), ALIGN)

    if bundle_offset > len(startup_payload):
      out.write(b"\x00" * (bundle_offset - len(startup_payload)))

    write_bundle(bundle_entries, out, base_offset=bundle_offset)

if __name__ == "__main__":
  main()
