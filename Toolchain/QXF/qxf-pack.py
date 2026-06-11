#!/usr/bin/env python3

"""
Packs an ELF32 binary, optional manifest, and optional resources into a
QXF container (.qx or .qapp).

Usage:
  qxf-pack.py <elf> -o <output> [-t qx|qapp] [-m <manifest.json>]
              [--imports LIB:NAME ...]

The --type flag or manifest JSON controls type, metadata, and resource
references. If neither is provided, a bare .qx (Generic) container is
produced.

Use --imports to declare QLL dependencies. Each argument is a pair of
QLL filename and canonical library name separated by a colon, e.g.
  --imports Core.qll:Quantum.Core Runtime.qll:Quantum.Runtime
"""

import argparse
import json
import os
import struct
import sys

# -- QXF constants -----------------------------------------------------------

QXF_MAGIC = b"QXF\x00"
QXF_VERSION = 1
QXF_HEADER_SIZE = 32

TYPE_MAP = {
  "generic": 1,
  "qx": 1,
  "app": 2,
  "application": 2,
  "qapp": 2,
}

FLAG_NEEDS_DISPLAY = 1 << 0
FLAG_PRIVILEGED    = 1 << 1

METADATA_KEYS = {
  "displayName":  1,
  "version":      2,
  "author":       3,
  "category":     4,
  "description":  5,
  "usageSummary": 6,
  "minOSVersion": 7,
}

CATEGORY_MAP = {
  "utility": 1,
  "system":  2,
  "game":    3,
}

RESOURCE_TYPES = {
  "icon":        1,
  "bitmap":      2,
  "string":      3,
  "font":        4,
  "raw":         5,
  "importtable": 6,
}

ICON_SIZE_MAP = {
  "16": 1,
  "32": 2,
  "48": 3,
  "64": 4,
}

RESOURCE_NAME_MAX = 32
RESOURCE_ENTRY_SIZE = 44  # 2+2+4+4+32

ALIGN = 4


IMPORT_NAME_MAX = 64


def align_up(value, alignment):
  return (value + alignment - 1) & ~(alignment - 1)


def pack_version(version_str):
  """Parse 'MAJOR.MINOR.PATCH' into a packed UInt32.

  Encoding: bits 31..22 = major (10 bits), bits 21..12 = minor (10 bits),
  bits 11..0 = patch (12 bits). Matches QLLVersion32::Make().
  """
  parts = version_str.split(".")

  if len(parts) != 3:
    print(
      f"Error: invalid version '{version_str}', expected MAJOR.MINOR.PATCH",
      file=sys.stderr,
    )
    sys.exit(1)

  major, minor, patch = int(parts[0]), int(parts[1]), int(parts[2])

  return ((major & 0x3FF) << 22) | ((minor & 0x3FF) << 12) | (patch & 0xFFF)


def pad_string(string, max_length):
  """Encode and zero-pad a string to a fixed byte length."""
  encoded = string.encode("ascii", errors="replace")
  encoded = encoded[:max_length - 1]
  encoded += b"\x00" * (max_length - len(encoded))

  return encoded


def build_import_table(imports):
  """Build the import table resource data from --imports arguments.

  Each import is a string "NAME:MAJOR.MINOR.PATCH" declaring a QLL
  dependency by canonical name and minimum version.
  """
  if not imports:
    return None

  dependencies = []

  for imp in imports:
    parts = imp.split(":", 1)

    if len(parts) != 2:
      print(
        f"Warning: invalid import '{imp}', expected NAME:VERSION",
        file=sys.stderr,
      )
      continue

    library_name, version_str = parts
    min_version = pack_version(version_str)

    dependencies.append({
      "name": library_name,
      "min_version": min_version,
    })

  if not dependencies:
    return None

  # build binary data matching QXFImportTableHeader + QXFImportDependency[]
  data = bytearray()

  # header: dependency count (u16) + reserved (u16)
  data += struct.pack("<HH", len(dependencies), 0)

  for dep in dependencies:
    # QXFImportDependency: Name[64] + MinVersion(u32) + SymbolCount(u32)
    data += pad_string(dep["name"], IMPORT_NAME_MAX)
    data += struct.pack("<I", dep["min_version"])
    data += struct.pack("<I", 0)  # SymbolCount = 0 for now

  return bytes(data)


def build_metadata(manifest):
  """Build the packed metadata section from manifest fields."""
  entries = bytearray()

  for key_name, key_id in METADATA_KEYS.items():
    if key_name not in manifest:
      continue

    value = manifest[key_name]

    if key_name == "category":
      category_id = CATEGORY_MAP.get(str(value).lower(), 0)
      value_bytes = struct.pack("<H", category_id)
    elif key_name == "minOSVersion":
      value_bytes = struct.pack("<I", int(value))
    else:
      value_bytes = str(value).encode("utf-8") + b"\x00"

    # entry header: key (u16) + length (u16)
    entries += struct.pack("<HH", key_id, len(value_bytes))
    entries += value_bytes

    # pad to 4-byte alignment
    padding = align_up(len(entries), ALIGN) - len(entries)
    entries += b"\x00" * padding

  return bytes(entries)


def build_resources(manifest, base_path):
  """Build resource table and data from manifest icon/resource entries."""
  table_entries = []
  resource_data = bytearray()

  # process icons
  icons = manifest.get("icons", {})

  for size_str, icon_path in icons.items():
    sub_type = ICON_SIZE_MAP.get(str(size_str), 0)

    if sub_type == 0:
      print(f"Warning: unknown icon size '{size_str}', skipping",
            file=sys.stderr)
      continue

    full_path = os.path.join(base_path, icon_path)

    if not os.path.exists(full_path):
      print(f"Warning: icon file not found: {full_path}", file=sys.stderr)
      continue

    with open(full_path, "rb") as f:
      data = f.read()

    name = os.path.basename(icon_path)

    table_entries.append({
      "type": RESOURCE_TYPES["icon"],
      "sub_type": sub_type,
      "data": data,
      "name": name,
    })

  # process generic resources
  resources = manifest.get("resources", [])

  for res in resources:
    res_type_str = res.get("type", "raw").lower()
    res_type = RESOURCE_TYPES.get(res_type_str, RESOURCE_TYPES["raw"])
    sub_type = int(res.get("subType", 0))
    res_path = res.get("path", "")
    name = res.get("name", os.path.basename(res_path))

    full_path = os.path.join(base_path, res_path)

    if not os.path.exists(full_path):
      print(f"Warning: resource file not found: {full_path}", file=sys.stderr)
      continue

    with open(full_path, "rb") as f:
      data = f.read()

    table_entries.append({
      "type": res_type,
      "sub_type": sub_type,
      "data": data,
      "name": name,
    })

  return table_entries


def main():
  parser = argparse.ArgumentParser(
    description="Pack an ELF32 binary into a QXF container."
  )
  parser.add_argument("elf", help="Path to the ELF32 binary")
  parser.add_argument("-o", "--output", required=True,
                      help="Output QXF file path")
  parser.add_argument("-m", "--manifest",
                      help="Path to manifest JSON file")
  parser.add_argument("-t", "--type",
                      choices=["qx", "qapp"],
                      help="Executable type (overrides manifest)")
  parser.add_argument("--needs-display", action="store_true",
                      help="Set NeedsDisplay flag")
  parser.add_argument("--privileged", action="store_true",
                      help="Set Privileged flag")
  parser.add_argument("--imports", nargs="*", metavar="FILE:NAME",
                      help="QLL dependencies (e.g. Core.qll:Quantum.Core)")

  args = parser.parse_args()

  # read ELF binary
  with open(args.elf, "rb") as f:
    elf_data = f.read()

  # verify ELF magic
  if elf_data[:4] != b"\x7fELF":
    print(f"Error: {args.elf} is not a valid ELF binary", file=sys.stderr)
    sys.exit(1)

  # load manifest or use defaults
  manifest = {}
  base_path = "."

  if args.manifest:
    with open(args.manifest, "r", encoding="utf-8") as f:
      manifest = json.load(f)

    base_path = os.path.dirname(os.path.abspath(args.manifest))

  # determine type (CLI --type overrides manifest)
  if args.type:
    type_str = args.type.lower()
  else:
    type_str = manifest.get("type", "generic").lower()

  qxf_type = TYPE_MAP.get(type_str, 1)

  # determine flags
  flags = 0

  if qxf_type == 2:  # application
    flags |= FLAG_NEEDS_DISPLAY

  # manifest or CLI can explicitly set display/privileged flags
  if manifest.get("needsDisplay", False) or args.needs_display:
    flags |= FLAG_NEEDS_DISPLAY

  if manifest.get("privileged", False) or args.privileged:
    flags |= FLAG_PRIVILEGED

  # build metadata section
  metadata_bytes = build_metadata(manifest)

  # build resource table and data
  resource_entries = build_resources(manifest, base_path)

  # build import table resource if --imports specified
  import_data = build_import_table(args.imports)

  if import_data is not None:
    resource_entries.append({
      "type": RESOURCE_TYPES["importtable"],
      "sub_type": 0,
      "data": import_data,
      "name": "imports",
    })

  resource_count = len(resource_entries)

  # compute layout offsets
  offset = QXF_HEADER_SIZE

  # metadata section
  metadata_offset = 0
  metadata_size = len(metadata_bytes)

  if metadata_size > 0:
    metadata_offset = offset
    offset += metadata_size
    offset = align_up(offset, ALIGN)

  # resource table
  resource_table_offset = 0

  if resource_count > 0:
    resource_table_offset = offset
    offset += resource_count * RESOURCE_ENTRY_SIZE
    offset = align_up(offset, ALIGN)

  # resource data
  resource_data_offsets = []

  for entry in resource_entries:
    resource_data_offsets.append(offset)
    offset += len(entry["data"])
    offset = align_up(offset, ALIGN)

  # ELF payload
  elf_offset = offset
  elf_size = len(elf_data)

  # build the output
  output = bytearray()

  # QXF header (32 bytes)
  header = struct.pack(
    "<4sHBBIIIIIHH",
    QXF_MAGIC,              # Magic
    QXF_VERSION,            # Version
    qxf_type,               # Type
    flags,                  # Flags
    elf_offset,             # ELFOffset
    elf_size,               # ELFSize
    metadata_offset,        # MetadataOffset
    metadata_size,          # MetadataSize
    resource_table_offset,  # ResourceTableOffset
    resource_count,         # ResourceCount
    0,                      # Reserved
  )
  output += header

  # metadata section
  if metadata_size > 0:
    output += b"\x00" * (metadata_offset - len(output))
    output += metadata_bytes
    padding = align_up(len(output), ALIGN) - len(output)
    output += b"\x00" * padding

  # resource table
  if resource_count > 0:
    output += b"\x00" * (resource_table_offset - len(output))

    for i, entry in enumerate(resource_entries):
      name_bytes = entry["name"].encode("ascii", errors="replace")
      name_bytes = name_bytes[:RESOURCE_NAME_MAX - 1]
      name_bytes += b"\x00" * (RESOURCE_NAME_MAX - len(name_bytes))

      table_entry = struct.pack(
        "<HHII",
        entry["type"],
        entry["sub_type"],
        resource_data_offsets[i],
        len(entry["data"]),
      )
      table_entry += name_bytes
      output += table_entry

    padding = align_up(len(output), ALIGN) - len(output)
    output += b"\x00" * padding

  # resource data
  for i, entry in enumerate(resource_entries):
    output += b"\x00" * (resource_data_offsets[i] - len(output))
    output += entry["data"]
    padding = align_up(len(output), ALIGN) - len(output)
    output += b"\x00" * padding

  # ELF payload
  output += b"\x00" * (elf_offset - len(output))
  output += elf_data

  # write output
  os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)

  with open(args.output, "wb") as f:
    f.write(output)

  ext = os.path.splitext(args.output)[1]
  type_name = {1: "Generic (.qx)",
               2: "Application (.qapp)"}.get(qxf_type, "Unknown")

  print(f"Packed {args.elf} -> {args.output}")
  print(f"  Type:      {type_name}")
  print(f"  Flags:     0x{flags:02x}")
  print(f"  Metadata:  {metadata_size} bytes")
  print(f"  Resources: {resource_count} entries")
  print(f"  ELF:       {elf_size} bytes at offset {elf_offset}")
  print(f"  Total:     {len(output)} bytes")


if __name__ == "__main__":
  main()
