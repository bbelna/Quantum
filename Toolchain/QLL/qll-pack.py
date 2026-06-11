#!/usr/bin/env python3

"""
Packs an ELF32 shared object into a QLL container (.qll).

Usage:
  qll-pack.py <elf-so> -o <output> --name <library-name>
              [--version MAJOR.MINOR.PATCH]
              [--arch ARCH] [--platform PLATFORM]
              [--min-os-version MAJOR.MINOR.PATCH]
              [--depends NAME:MAJOR.MINOR.PATCH ...]

Reads the ELF32 .so file, extracts exported symbols from the .dynsym
table, and produces a QLL container with the library identity, export
table, string table, dependency table, and ELF payload.
"""

import argparse
import os
import struct
import sys

# -- QLL constants -----------------------------------------------------------

QLL_MAGIC = b"QLL\x00"
QLL_FORMAT_VERSION = 1
QLL_EXPORT_ENTRY_SIZE = 12
QLL_DEPENDENCY_ENTRY_SIZE = 68  # 64 (name) + 4 (min version)

# Identity field sizes (must match QLL.hpp)
QLL_NAME_MAX = 64
QLL_ARCH_MAX = 16
QLL_PLATFORM_MAX = 16

# Header: Magic(4) + FormatVersion(2) + Flags(2) + Identity(104) +
#   ELF{Offset,Size}(8) + Export{Offset,Count}(8) +
#   String{Offset,Size}(8) + Dep{Offset}(4) + DepCount(2) + Reserved(2)
QLL_HEADER_SIZE = 144

ALIGN = 4

# -- ELF constants -----------------------------------------------------------

ELF_MAGIC = b"\x7fELF"

SHT_DYNSYM = 11
SHT_SYMTAB = 2
SHT_STRTAB = 3

STB_GLOBAL = 1
STB_WEAK = 2

STT_FUNC = 2
STT_OBJECT = 1

STV_DEFAULT = 0

SHN_UNDEF = 0


# -- Utilities ---------------------------------------------------------------

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

  if major > 1023 or minor > 1023 or patch > 4095:
    print(
      f"Error: version component out of range in '{version_str}'",
      file=sys.stderr,
    )
    sys.exit(1)

  return ((major & 0x3FF) << 22) | ((minor & 0x3FF) << 12) | (patch & 0xFFF)


def unpack_version(packed):
  """Convert a packed version UInt32 back to a string."""
  major = (packed >> 22) & 0x3FF
  minor = (packed >> 12) & 0x3FF
  patch = packed & 0xFFF

  return f"{major}.{minor}.{patch}"


def pad_string(string, max_length):
  """Encode and zero-pad a string to a fixed byte length."""
  encoded = string.encode("ascii", errors="replace")
  encoded = encoded[:max_length - 1]
  encoded += b"\x00" * (max_length - len(encoded))

  return encoded


def djb2_hash(name):
  """Compute djb2 hash matching QLLHashName in QLL.hpp."""
  h = 5381

  for ch in name:
    h = ((h << 5) + h + ch) & 0xFFFFFFFF

  return h


# -- ELF parsing -------------------------------------------------------------

def parse_elf_sections(data):
  """Parse ELF32 section headers and return them as a list of dicts."""
  if data[:4] != ELF_MAGIC:
    return None

  (
    e_shoff,
    e_shentsize,
    e_shnum,
    e_shstrndx,
  ) = struct.unpack_from("<I", data, 32)[0], \
      struct.unpack_from("<H", data, 46)[0], \
      struct.unpack_from("<H", data, 48)[0], \
      struct.unpack_from("<H", data, 50)[0]

  sections = []

  for i in range(e_shnum):
    offset = e_shoff + i * e_shentsize
    sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link = \
      struct.unpack_from("<IIIIIII", data, offset)

    sh_entsize = struct.unpack_from("<I", data, offset + 36)[0]

    sections.append({
      "name_offset": sh_name,
      "type": sh_type,
      "flags": sh_flags,
      "addr": sh_addr,
      "offset": sh_offset,
      "size": sh_size,
      "link": sh_link,
      "entsize": sh_entsize,
      "index": i,
    })

  return sections, e_shstrndx


def get_string(data, strtab_section, offset):
  """Get a null-terminated string from a string table section."""
  start = strtab_section["offset"] + offset
  end = data.index(b"\x00", start)

  return data[start:end]


def extract_exports(data, sections, shstrndx):
  """Extract global/weak symbols from .dynsym or .symtab."""
  exports = []

  # prefer .dynsym, fall back to .symtab
  symtab = None

  for section in sections:
    if section["type"] == SHT_DYNSYM:
      symtab = section
      break

  if symtab is None:
    for section in sections:
      if section["type"] == SHT_SYMTAB:
        symtab = section
        break

  if symtab is None:
    return exports

  strtab = sections[symtab["link"]]
  entry_size = symtab["entsize"]

  if entry_size == 0:
    entry_size = 16

  entry_count = symtab["size"] // entry_size

  for i in range(entry_count):
    offset = symtab["offset"] + i * entry_size

    st_name, st_value, st_size, st_info, st_other, st_shndx = \
      struct.unpack_from("<IIIBBH", data, offset)

    binding = st_info >> 4
    sym_type = st_info & 0xF
    visibility = st_other & 0x3

    if st_shndx == SHN_UNDEF:
      continue

    if binding not in (STB_GLOBAL, STB_WEAK):
      continue

    if visibility != STV_DEFAULT:
      continue

    if st_name == 0:
      continue

    name = get_string(data, strtab, st_name)

    if name.startswith(b"_GLOBAL_OFFSET_TABLE_"):
      continue

    if name.startswith(b"_DYNAMIC"):
      continue

    is_function = sym_type == STT_FUNC
    flags = 1 if is_function else 0

    exports.append({
      "name": name,
      "value": st_value,
      "flags": flags,
    })

  exports.sort(key=lambda e: e["name"])

  return exports


# -- Main --------------------------------------------------------------------

def main():
  parser = argparse.ArgumentParser(
    description="Pack an ELF32 shared object into a QLL container."
  )
  parser.add_argument("elf", help="Path to the ELF32 shared object (.so)")
  parser.add_argument("-o", "--output", required=True,
                      help="Output QLL file path")
  parser.add_argument("--name", required=True,
                      help="Canonical library name (e.g. 'Quantum.Core')")
  parser.add_argument("--version", default="0.1.0",
                      help="Library version (MAJOR.MINOR.PATCH)")
  parser.add_argument("--arch", default="IA32",
                      help="Target architecture (e.g. 'IA32', 'x86_64')")
  parser.add_argument("--platform", default="PC",
                      help="Target platform (e.g. 'PC', 'RPi')")
  parser.add_argument("--min-os-version", default="0.0.0",
                      help="Minimum QuantumOS version (MAJOR.MINOR.PATCH)")
  parser.add_argument("--depends", nargs="*", metavar="NAME:VERSION",
                      help="QLL dependencies (e.g. 'Quantum.Core:0.1.0')")

  args = parser.parse_args()

  # read ELF
  with open(args.elf, "rb") as f:
    elf_data = f.read()

  if elf_data[:4] != ELF_MAGIC:
    print(f"Error: {args.elf} is not a valid ELF binary", file=sys.stderr)
    sys.exit(1)

  e_type = struct.unpack_from("<H", elf_data, 16)[0]

  if e_type != 3:
    print(
      f"Warning: ELF type is {e_type} (expected 3 = ET_DYN). "
      "Proceeding anyway.",
      file=sys.stderr,
    )

  # parse sections and extract exports
  result = parse_elf_sections(elf_data)

  if result is None:
    print("Error: failed to parse ELF sections", file=sys.stderr)
    sys.exit(1)

  sections, shstrndx = result
  exports = extract_exports(elf_data, sections, shstrndx)

  if not exports:
    print("Warning: no exported symbols found", file=sys.stderr)

  # build string table
  string_table = bytearray()
  export_entries = []

  for export in exports:
    name_offset = len(string_table)
    string_table += export["name"] + b"\x00"

    export_entries.append({
      "name_offset": name_offset,
      "value": export["value"],
      "flags": export["flags"],
    })

  # parse dependencies
  dependencies = []

  if args.depends:
    for dep_str in args.depends:
      parts = dep_str.split(":", 1)

      if len(parts) != 2:
        print(
          f"Error: invalid dependency '{dep_str}', "
          "expected NAME:MAJOR.MINOR.PATCH",
          file=sys.stderr,
        )
        sys.exit(1)

      dep_name, dep_version = parts

      dependencies.append({
        "name": dep_name,
        "min_version": pack_version(dep_version),
      })

  # pack identity fields
  library_version = pack_version(args.version)
  min_os_version = pack_version(args.min_os_version)

  has_init = any(e["name"] == b"__qll_init" for e in exports)
  flags = 1 if has_init else 0

  # compute layout
  offset = QLL_HEADER_SIZE

  # export table
  export_table_offset = 0
  export_count = len(export_entries)

  if export_count > 0:
    export_table_offset = offset
    offset += export_count * QLL_EXPORT_ENTRY_SIZE
    offset = align_up(offset, ALIGN)

  # string table
  string_table_offset = 0
  string_table_size = len(string_table)

  if string_table_size > 0:
    string_table_offset = offset
    offset += string_table_size
    offset = align_up(offset, ALIGN)

  # dependency table
  dependency_table_offset = 0
  dependency_count = len(dependencies)

  if dependency_count > 0:
    dependency_table_offset = offset
    offset += dependency_count * QLL_DEPENDENCY_ENTRY_SIZE
    offset = align_up(offset, ALIGN)

  # ELF payload
  elf_offset = offset
  elf_size = len(elf_data)

  # build output
  output = bytearray()

  # pack identity block (104 bytes)
  identity = bytearray()
  identity += pad_string(args.name, QLL_NAME_MAX)         # Name[64]
  identity += struct.pack("<I", library_version)           # LibraryVersion
  identity += pad_string(args.arch, QLL_ARCH_MAX)          # Architecture[16]
  identity += pad_string(args.platform, QLL_PLATFORM_MAX)  # Platform[16]
  identity += struct.pack("<I", min_os_version)            # MinOSVersion
  assert len(identity) == 104

  # QLL header
  header = bytearray()
  header += QLL_MAGIC                                      # Magic[4]
  header += struct.pack("<H", QLL_FORMAT_VERSION)          # FormatVersion
  header += struct.pack("<H", flags)                       # Flags
  header += identity                                       # Identity (104)
  header += struct.pack("<I", elf_offset)                  # ELFOffset
  header += struct.pack("<I", elf_size)                    # ELFSize
  header += struct.pack("<I", export_table_offset)         # ExportTableOffset
  header += struct.pack("<I", export_count)                # ExportCount
  header += struct.pack("<I", string_table_offset)         # StringTableOffset
  header += struct.pack("<I", string_table_size)           # StringTableSize
  header += struct.pack("<I", dependency_table_offset)     # DependencyTableOffset
  header += struct.pack("<H", dependency_count)            # DependencyCount
  header += struct.pack("<H", 0)                           # Reserved
  assert len(header) == QLL_HEADER_SIZE, \
    f"Header size mismatch: {len(header)} != {QLL_HEADER_SIZE}"

  output += header

  # export table
  if export_count > 0:
    output += b"\x00" * (export_table_offset - len(output))

    for entry in export_entries:
      output += struct.pack(
        "<IIHH",
        entry["name_offset"],
        entry["value"],
        entry["flags"],
        0,
      )

    padding = align_up(len(output), ALIGN) - len(output)
    output += b"\x00" * padding

  # string table
  if string_table_size > 0:
    output += b"\x00" * (string_table_offset - len(output))
    output += string_table

    padding = align_up(len(output), ALIGN) - len(output)
    output += b"\x00" * padding

  # dependency table
  if dependency_count > 0:
    output += b"\x00" * (dependency_table_offset - len(output))

    for dep in dependencies:
      output += pad_string(dep["name"], QLL_NAME_MAX)
      output += struct.pack("<I", dep["min_version"])

    padding = align_up(len(output), ALIGN) - len(output)
    output += b"\x00" * padding

  # ELF payload
  output += b"\x00" * (elf_offset - len(output))
  output += elf_data

  # write output
  os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)

  with open(args.output, "wb") as f:
    f.write(output)

  version_str = unpack_version(library_version)

  print(f"Packed {args.elf} -> {args.output}")
  print(f"  Name:      {args.name}")
  print(f"  Version:   {version_str}")
  print(f"  Arch:      {args.arch}")
  print(f"  Platform:  {args.platform}")
  print(f"  Flags:     0x{flags:04x}")
  print(f"  Exports:   {export_count} symbols")
  print(f"  StrTable:  {string_table_size} bytes")
  print(f"  Depends:   {dependency_count} libraries")
  print(f"  ELF:       {elf_size} bytes at offset {elf_offset}")
  print(f"  Total:     {len(output)} bytes")


if __name__ == "__main__":
  main()
