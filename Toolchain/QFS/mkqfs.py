#!/usr/bin/env python3

"""
Creates a QFS (Quantum File System) image from a directory tree.

Usage:
  mkqfs.py -o <output.qfs> -s <size_mb> [--label LABEL] [--from-dir DIR]
  mkqfs.py -o <output.qfs> -s <size_mb> [--label LABEL] [--empty]

Examples:
  mkqfs.py -o system.qfs -s 32 --label "System" --from-dir ./sysroot
  mkqfs.py -o blank.qfs -s 16 --label "Data" --empty
"""

import argparse
import math
import os
import struct
import sys
import time
import uuid

# -- Constants (must match Format.hpp) -----------------------------------------

BLOCK_SIZE = 4096
BLOCK_SIZE_LOG2 = 12
INODE_SIZE = 256
DEFAULT_BLOCKS_PER_GROUP = 8192
DEFAULT_INODES_PER_GROUP = 1024

# Magic bytes
SUPERBLOCK_MAGIC = b"QFS1\x00\x00\x00\x00"
JOURNAL_MAGIC = b"QJNL"
JOURNAL_DESC_MAGIC = b"QDSC"
JOURNAL_COMMIT_MAGIC = b"QCMT"

# Reserved inodes
NULL_INODE = 0
JOURNAL_INODE = 1
ROOT_INODE = 2
FIRST_USER_INODE = 3

# Feature flags
FEAT_JOURNAL = 0x1
FEAT_EXTENTS = 0x2
FEAT_DIR_BTREE = 0x4
FEAT_META_CSUM = 0x8

# IndexNode types (bits 15..12 of Mode)
ITYPE_REGULAR = 0x1000
ITYPE_DIRECTORY = 0x2000
ITYPE_SYMLINK = 0x3000

# IndexNode flags
IFLAG_INLINE_DATA = 0x1
IFLAG_EXTENT_TREE = 0x2
IFLAG_DIR_BTREE = 0x4

# Directory file types
FTYPE_UNKNOWN = 0
FTYPE_REGULAR = 1
FTYPE_DIRECTORY = 2
FTYPE_SYMLINK = 3

# Checksum algorithm
CSUM_NONE = 0
CSUM_CRC32 = 1

# Volume state
STATE_CLEAN = 0
STATE_MOUNTED = 1
STATE_ERRORS = 2

# Inline data limit
MAX_INLINE_DATA = 100
MAX_INLINE_EXTENTS = 4
MAX_FILENAME = 255


# -- CRC32 --------------------------------------------------------------------

_CRC32_TABLE = None


def _init_crc32_table():
  global _CRC32_TABLE

  if _CRC32_TABLE is not None:
    return

  _CRC32_TABLE = []

  for i in range(256):
    remainder = i

    for _ in range(8):
      if remainder & 1:
        remainder = (remainder >> 1) ^ 0xEDB88320
      else:
        remainder >>= 1

    _CRC32_TABLE.append(remainder)


def crc32(data):
  """Compute CRC32 matching the C++ implementation in CRC32.hpp."""
  _init_crc32_table()

  crc = 0xFFFFFFFF

  for byte in data:
    index = (crc ^ byte) & 0xFF
    crc = (crc >> 8) ^ _CRC32_TABLE[index]

  return crc ^ 0xFFFFFFFF


def crc32_with_hole(data, hole_offset, hole_size):
  """Compute CRC32 treating a region as zeros (for self-checksumming)."""
  patched = bytearray(data)
  patched[hole_offset:hole_offset + hole_size] = b"\x00" * hole_size

  return crc32(bytes(patched))


# -- FNV-1a hash ---------------------------------------------------------------

def fnv1a_hash(name_bytes):
  """FNV-1a 32-bit hash with ASCII lowercasing, matching Format.hpp."""
  h = 0x811C9DC5

  for b in name_bytes:
    if 0x41 <= b <= 0x5A:  # A-Z
      b += 0x20

    h ^= b
    h = (h * 0x01000193) & 0xFFFFFFFF

  return h


# -- Utilities -----------------------------------------------------------------

def align_up(value, alignment):
  return (value + alignment - 1) & ~(alignment - 1)


def blocks_for_bytes(size):
  return align_up(size, BLOCK_SIZE) // BLOCK_SIZE


def pad_to_block(data):
  """Pad data to block size boundary."""
  remainder = len(data) % BLOCK_SIZE

  if remainder != 0:
    data += b"\x00" * (BLOCK_SIZE - remainder)

  return data


# -- Image Builder -------------------------------------------------------------

class QFSImage:
  """Builds a QFS image in memory."""

  def __init__(self, total_blocks, label="System"):
    self.total_blocks = total_blocks
    self.label = label
    self.data = bytearray(total_blocks * BLOCK_SIZE)
    self.now = int(time.time())
    self.volume_uuid = uuid.uuid4().bytes

    # Compute geometry
    self.blocks_per_group = min(DEFAULT_BLOCKS_PER_GROUP, total_blocks)
    self.block_group_count = max(
      1, math.ceil(total_blocks / self.blocks_per_group)
    )
    self.inodes_per_group = min(
      DEFAULT_INODES_PER_GROUP,
      max(64, total_blocks // 4)  # at least 64, roughly 1 per 4 blocks
    )
    self.total_inodes = self.inodes_per_group * self.block_group_count

    # Journal sizing
    if total_blocks >= 16384:  # >= 64MB
      self.journal_blocks = 256
    elif total_blocks >= 1024:  # >= 4MB
      self.journal_blocks = 32
    else:
      self.journal_blocks = 8

    # Layout computation
    self.journal_start = 3  # after superblock + backup
    self.bgdt_start = self.journal_start + self.journal_blocks
    self.bgdt_blocks = blocks_for_bytes(
      self.block_group_count * 64  # sizeof(BlockGroupDescriptor)
    )
    self.first_data_block = self.bgdt_start + self.bgdt_blocks

    # Block group layout
    self.inode_table_blocks_per_group = blocks_for_bytes(
      self.inodes_per_group * INODE_SIZE
    )

    # Per-group: 1 block bitmap + 1 inode bitmap + inode table + data
    self.group_overhead = 2 + self.inode_table_blocks_per_group

    # Allocation state
    self.block_bitmaps = []  # list of bytearray, one per group
    self.inode_bitmaps = []  # list of bytearray, one per group
    self.next_inode = FIRST_USER_INODE
    self.free_blocks = 0
    self.free_inodes = 0
    self.group_descriptors = []

    self._init_groups()

  def _init_groups(self):
    """Initialize block group structures."""
    current_block = self.first_data_block

    for group_index in range(self.block_group_count):
      # How many blocks in this group
      group_start = current_block
      remaining = self.total_blocks - group_start
      group_blocks = min(self.blocks_per_group, remaining)

      if group_blocks < self.group_overhead + 1:
        # Not enough room for this group
        self.block_group_count = group_index
        break

      bitmap_block = group_start
      inode_bitmap_block = group_start + 1
      inode_table_block = group_start + 2
      data_start = group_start + self.group_overhead

      data_blocks = group_blocks - self.group_overhead

      # Block bitmap (1 block = 32768 bits, covers up to 32768 blocks)
      block_bitmap = bytearray(BLOCK_SIZE)

      # Mark overhead blocks as used in bitmap
      for i in range(self.group_overhead):
        block_bitmap[i // 8] |= (1 << (i % 8))

      # Mark blocks beyond group as used
      for i in range(group_blocks, self.blocks_per_group):
        if i < BLOCK_SIZE * 8:
          block_bitmap[i // 8] |= (1 << (i % 8))

      self.block_bitmaps.append(block_bitmap)

      # IndexNode bitmap
      inode_bitmap = bytearray(BLOCK_SIZE)

      # Mark reserved inodes as used in the first group
      if group_index == 0:
        for i in range(FIRST_USER_INODE):
          inode_bitmap[i // 8] |= (1 << (i % 8))

      self.inode_bitmaps.append(inode_bitmap)

      # Group descriptor
      free_blocks = data_blocks
      free_inodes = self.inodes_per_group

      if group_index == 0:
        free_inodes -= FIRST_USER_INODE

      self.free_blocks += free_blocks
      self.free_inodes += free_inodes

      self.group_descriptors.append({
        "block_bitmap": bitmap_block,
        "inode_bitmap": inode_bitmap_block,
        "inode_table": inode_table_block,
        "free_blocks": free_blocks,
        "free_inodes": free_inodes,
        "directory_count": 0,
        "group_start": group_start,
        "group_blocks": group_blocks,
        "data_start": data_start,
      })

      current_block += group_blocks

    self.total_inodes = self.inodes_per_group * self.block_group_count

  def _allocate_block(self, preferred_group=0):
    """Allocate a single block, returns block number or None."""
    for offset in range(self.block_group_count):
      group_index = (preferred_group + offset) % self.block_group_count
      group = self.group_descriptors[group_index]

      if group["free_blocks"] <= 0:
        continue

      bitmap = self.block_bitmaps[group_index]

      for bit in range(group["group_blocks"]):
        byte_index = bit // 8
        bit_index = bit % 8

        if byte_index >= BLOCK_SIZE:
          break

        if not (bitmap[byte_index] & (1 << bit_index)):
          bitmap[byte_index] |= (1 << bit_index)
          group["free_blocks"] -= 1
          self.free_blocks -= 1

          return group["group_start"] + bit

    return None

  def _allocate_blocks(self, count, preferred_group=0):
    """Allocate multiple blocks, returns list of block numbers."""
    blocks = []

    for _ in range(count):
      block = self._allocate_block(preferred_group)

      if block is None:
        return None

      blocks.append(block)

    return blocks

  def _allocate_inode(self, preferred_group=0):
    """Allocate an inode, returns inode number or None."""
    for offset in range(self.block_group_count):
      group_index = (preferred_group + offset) % self.block_group_count
      group = self.group_descriptors[group_index]

      if group["free_inodes"] <= 0:
        continue

      bitmap = self.inode_bitmaps[group_index]
      base_inode = group_index * self.inodes_per_group

      for bit in range(self.inodes_per_group):
        byte_index = bit // 8
        bit_index = bit % 8

        if not (bitmap[byte_index] & (1 << bit_index)):
          bitmap[byte_index] |= (1 << bit_index)
          group["free_inodes"] -= 1
          self.free_inodes -= 1

          return base_inode + bit

    return None

  def _write_block(self, block_number, data):
    """Write data to a block."""
    offset = block_number * BLOCK_SIZE

    if len(data) > BLOCK_SIZE:
      data = data[:BLOCK_SIZE]

    self.data[offset:offset + len(data)] = data

  def _read_block(self, block_number):
    """Read a block."""
    offset = block_number * BLOCK_SIZE

    return bytes(self.data[offset:offset + BLOCK_SIZE])

  def _write_inode(self, inode_number, inode_data):
    """Write an inode to the inode table."""
    group_index = inode_number // self.inodes_per_group
    local_index = inode_number % self.inodes_per_group
    group = self.group_descriptors[group_index]
    table_block = group["inode_table"]
    inodes_per_block = BLOCK_SIZE // INODE_SIZE
    block_offset = local_index // inodes_per_block
    offset_in_block = (local_index % inodes_per_block) * INODE_SIZE
    absolute_offset = (table_block + block_offset) * BLOCK_SIZE + offset_in_block

    if len(inode_data) != INODE_SIZE:
      raise ValueError(f"IndexNode data must be {INODE_SIZE} bytes")

    self.data[absolute_offset:absolute_offset + INODE_SIZE] = inode_data

  def _pack_inode(
    self, inode_number, mode, size, flags=0, link_count=1,
    extents=None, dir_btree_root=0, dir_entry_count=0, inline_data=None
  ):
    """Pack an inode structure."""
    inode = bytearray(INODE_SIZE)

    # Mode + LinkCount + UID + GID
    struct.pack_into("<HHII", inode, 0, mode, link_count, 0, 0)

    # Flags + SizeBytes + SizeHigh + BlockCount
    block_count = 0

    if extents:
      for ext in extents:
        block_count += ext[2]  # length

    struct.pack_into("<IIII", inode, 12, flags, size, 0, block_count)

    # Timestamps
    struct.pack_into("<IIII", inode, 28, self.now, self.now, self.now, 0)

    # Generation
    struct.pack_into("<I", inode, 44, 1)

    # Extents
    extent_count = 0

    if extents and len(extents) <= MAX_INLINE_EXTENTS:
      extent_count = len(extents)

      for i, (logical, physical, length) in enumerate(extents):
        struct.pack_into(
          "<IIII", inode, 56 + i * 16,
          logical, physical, length, 0
        )

    struct.pack_into("<I", inode, 48, extent_count)

    # ExtentTreeRoot (0 for inline)
    struct.pack_into("<I", inode, 52, 0)

    # DirectoryBTreeRoot + EntryCount
    struct.pack_into("<II", inode, 120, dir_btree_root, dir_entry_count)

    # Inline data
    if inline_data and (flags & IFLAG_INLINE_DATA):
      inode[128:128 + len(inline_data)] = inline_data

    # IndexNodeNumber (self-reference)
    struct.pack_into("<I", inode, 248, inode_number)

    # Checksum
    checksum = crc32_with_hole(bytes(inode), 252, 4)
    struct.pack_into("<I", inode, 252, checksum)

    return bytes(inode)

  def _pack_dir_entry(self, inode_number, name_bytes, file_type):
    """Pack a directory entry."""
    name_hash = fnv1a_hash(name_bytes)
    name_len = len(name_bytes)
    record_len = align_up(12 + name_len, 4)

    entry = bytearray(record_len)
    struct.pack_into("<IIHBB", entry, 0,
                     inode_number, name_hash, record_len, name_len, file_type)
    entry[12:12 + name_len] = name_bytes

    return bytes(entry)

  def _create_dir_btree_leaf(self, entries, next_leaf=0):
    """Create a B-tree leaf node from directory entries.

    entries: list of packed DirectoryEntry byte strings
    Returns the block number of the allocated leaf node.
    """
    block_num = self._allocate_block()

    if block_num is None:
      raise RuntimeError("Out of blocks for directory B-tree")

    node = bytearray(BLOCK_SIZE)

    # Header: Level=0, EntryCount, FreeBytes, Reserved, Checksum, NextLeaf
    total_entry_bytes = sum(len(e) for e in entries)
    free_bytes = BLOCK_SIZE - 16 - total_entry_bytes

    struct.pack_into("<HHHH", node, 0, 0, len(entries), free_bytes, 0)
    struct.pack_into("<I", node, 12, next_leaf)

    # Pack entries
    offset = 16

    # Sort by name hash
    def entry_hash(e):
      return struct.unpack_from("<I", e, 4)[0]

    entries_sorted = sorted(entries, key=entry_hash)

    for entry in entries_sorted:
      node[offset:offset + len(entry)] = entry
      offset += len(entry)

    # Checksum
    checksum = crc32_with_hole(bytes(node), 8, 4)
    struct.pack_into("<I", node, 8, checksum)

    self._write_block(block_num, bytes(node))

    return block_num

  def _create_root_directory(self):
    """Create the root directory inode and its initial B-tree."""
    # Root directory has . and .. pointing to itself
    dot_entry = self._pack_dir_entry(ROOT_INODE, b".", FTYPE_DIRECTORY)
    dotdot_entry = self._pack_dir_entry(ROOT_INODE, b"..", FTYPE_DIRECTORY)

    btree_root = self._create_dir_btree_leaf([dot_entry, dotdot_entry])

    root_inode = self._pack_inode(
      ROOT_INODE,
      mode=ITYPE_DIRECTORY | 0o755,
      size=0,
      flags=IFLAG_DIR_BTREE,
      link_count=2,
      dir_btree_root=btree_root,
      dir_entry_count=2,
    )

    self._write_inode(ROOT_INODE, root_inode)
    self.group_descriptors[0]["directory_count"] += 1

  def add_file(self, path, file_data, parent_inode=ROOT_INODE):
    """Add a file to the filesystem at the given path.

    For now, only supports adding to the root directory or creating
    a single level of subdirectories.
    """
    parts = path.strip("/").split("/")

    if len(parts) == 1:
      return self._add_file_to_dir(parent_inode, parts[0], file_data)

    # Need to create/find subdirectories
    current_inode = parent_inode

    for dir_name in parts[:-1]:
      existing = self._find_in_directory(current_inode, dir_name)

      if existing is not None:
        current_inode = existing
      else:
        current_inode = self._create_directory(current_inode, dir_name)

    return self._add_file_to_dir(current_inode, parts[-1], file_data)

  def _add_file_to_dir(self, parent_inode, name, file_data):
    """Add a file directly to a directory."""
    inode_number = self._allocate_inode()

    if inode_number is None:
      raise RuntimeError(f"Out of inodes adding '{name}'")

    name_bytes = name.encode("utf-8")

    if len(name_bytes) > MAX_FILENAME:
      raise ValueError(f"Filename too long: '{name}'")

    size = len(file_data)

    # Determine storage strategy
    if size <= MAX_INLINE_DATA:
      # Inline data
      inode = self._pack_inode(
        inode_number,
        mode=ITYPE_REGULAR | 0o644,
        size=size,
        flags=IFLAG_INLINE_DATA,
        inline_data=file_data,
      )
    else:
      # Allocate blocks
      needed_blocks = blocks_for_bytes(size)
      blocks = self._allocate_blocks(needed_blocks)

      if blocks is None:
        raise RuntimeError(f"Out of blocks adding '{name}'")

      # Write data to blocks
      for i, block_num in enumerate(blocks):
        start = i * BLOCK_SIZE
        end = min(start + BLOCK_SIZE, size)
        block_data = file_data[start:end]

        if len(block_data) < BLOCK_SIZE:
          block_data = block_data + b"\x00" * (BLOCK_SIZE - len(block_data))

        self._write_block(block_num, block_data)

      # Build extents from contiguous runs
      extents = self._blocks_to_extents(blocks)

      inode = self._pack_inode(
        inode_number,
        mode=ITYPE_REGULAR | 0o644,
        size=size,
        extents=extents,
      )

    self._write_inode(inode_number, inode)

    # Add entry to parent directory
    self._add_entry_to_directory(parent_inode, inode_number, name_bytes,
                                 FTYPE_REGULAR)

    return inode_number

  def _create_directory(self, parent_inode, name):
    """Create a subdirectory."""
    inode_number = self._allocate_inode()

    if inode_number is None:
      raise RuntimeError(f"Out of inodes creating directory '{name}'")

    name_bytes = name.encode("utf-8")

    # Create . and .. entries
    dot_entry = self._pack_dir_entry(inode_number, b".", FTYPE_DIRECTORY)
    dotdot_entry = self._pack_dir_entry(parent_inode, b"..", FTYPE_DIRECTORY)

    btree_root = self._create_dir_btree_leaf([dot_entry, dotdot_entry])

    inode = self._pack_inode(
      inode_number,
      mode=ITYPE_DIRECTORY | 0o755,
      size=0,
      flags=IFLAG_DIR_BTREE,
      link_count=2,
      dir_btree_root=btree_root,
      dir_entry_count=2,
    )

    self._write_inode(inode_number, inode)

    # Find which group this inode is in
    group_index = inode_number // self.inodes_per_group
    self.group_descriptors[group_index]["directory_count"] += 1

    # Add entry to parent
    self._add_entry_to_directory(parent_inode, inode_number, name_bytes,
                                 FTYPE_DIRECTORY)

    # Increment parent's link count (for ..)
    self._increment_link_count(parent_inode)

    return inode_number

  def _add_entry_to_directory(self, dir_inode, child_inode, name_bytes,
                               file_type):
    """Add an entry to an existing directory's B-tree.

    For mkqfs, we use a simplified approach: read the existing leaf,
    add the entry, and if it overflows, split into a new leaf.
    """
    # Read directory inode to get B-tree root
    inode_data = self._read_inode(dir_inode)
    btree_root = struct.unpack_from("<I", inode_data, 120)[0]
    entry_count = struct.unpack_from("<I", inode_data, 124)[0]

    new_entry = self._pack_dir_entry(child_inode, name_bytes, file_type)

    # Read the leaf node
    leaf_data = bytearray(self._read_block(btree_root))
    level, leaf_entries, free_bytes = struct.unpack_from("<HHH", leaf_data, 0)

    if free_bytes >= len(new_entry):
      # Fits in existing leaf — add it
      # Find insertion point (collect existing entries, add new one, re-sort)
      entries = self._extract_leaf_entries(leaf_data, leaf_entries)
      entries.append(new_entry)
      entries.sort(key=lambda e: struct.unpack_from("<I", e, 4)[0])

      # Rewrite leaf
      total_bytes = sum(len(e) for e in entries)
      new_free = BLOCK_SIZE - 16 - total_bytes
      next_leaf = struct.unpack_from("<I", leaf_data, 12)[0]

      new_leaf = bytearray(BLOCK_SIZE)
      struct.pack_into("<HHHH", new_leaf, 0,
                       0, len(entries), new_free, 0)
      struct.pack_into("<I", new_leaf, 12, next_leaf)

      offset = 16

      for entry in entries:
        new_leaf[offset:offset + len(entry)] = entry
        offset += len(entry)

      checksum = crc32_with_hole(bytes(new_leaf), 8, 4)
      struct.pack_into("<I", new_leaf, 8, checksum)

      self._write_block(btree_root, bytes(new_leaf))
    else:
      # Need to split — for simplicity in mkqfs, just create a new leaf
      # and link them. A real implementation would create internal nodes.
      entries = self._extract_leaf_entries(leaf_data, leaf_entries)
      entries.append(new_entry)
      entries.sort(key=lambda e: struct.unpack_from("<I", e, 4)[0])

      # Split in half
      mid = len(entries) // 2
      left_entries = entries[:mid]
      right_entries = entries[mid:]

      # Right leaf
      right_block = self._create_dir_btree_leaf(right_entries, next_leaf=0)

      # Left leaf (rewrite the original)
      total_left = sum(len(e) for e in left_entries)
      new_free = BLOCK_SIZE - 16 - total_left

      new_leaf = bytearray(BLOCK_SIZE)
      struct.pack_into("<HHHH", new_leaf, 0,
                       0, len(left_entries), new_free, 0)
      struct.pack_into("<I", new_leaf, 12, right_block)

      offset = 16

      for entry in left_entries:
        new_leaf[offset:offset + len(entry)] = entry
        offset += len(entry)

      checksum = crc32_with_hole(bytes(new_leaf), 8, 4)
      struct.pack_into("<I", new_leaf, 8, checksum)

      self._write_block(btree_root, bytes(new_leaf))

      # Create internal root
      # Get the first hash of the right leaf
      right_first_hash = struct.unpack_from("<I", right_entries[0], 4)[0]

      internal = bytearray(BLOCK_SIZE)
      # Level=1, EntryCount=2 (two child pointers)
      struct.pack_into("<HHHH", internal, 0, 1, 2, 0, 0)
      struct.pack_into("<I", internal, 12, 0)  # NextLeaf=0 (not a leaf)

      # First child (left) — covers all hashes up to right_first_hash
      struct.pack_into("<II", internal, 16, 0, btree_root)

      # Second child (right) — covers hashes >= right_first_hash
      struct.pack_into("<II", internal, 24, right_first_hash, right_block)

      int_checksum = crc32_with_hole(bytes(internal), 8, 4)
      struct.pack_into("<I", internal, 8, int_checksum)

      # Allocate new block for internal root
      new_root_block = self._allocate_block()

      if new_root_block is None:
        raise RuntimeError("Out of blocks for B-tree internal node")

      self._write_block(new_root_block, bytes(internal))

      # Update inode's btree root
      inode_data = bytearray(self._read_inode(dir_inode))
      struct.pack_into("<I", inode_data, 120, new_root_block)

      # Recompute inode checksum
      checksum = crc32_with_hole(bytes(inode_data), 252, 4)
      struct.pack_into("<I", inode_data, 252, checksum)

      self._write_inode(dir_inode, bytes(inode_data))

      # We need to return early since we already updated the btree root
      # Update entry count
      inode_data = bytearray(self._read_inode(dir_inode))
      struct.pack_into("<I", inode_data, 124, entry_count + 1)

      checksum = crc32_with_hole(bytes(inode_data), 252, 4)
      struct.pack_into("<I", inode_data, 252, checksum)

      self._write_inode(dir_inode, bytes(inode_data))

      return

    # Update directory entry count
    inode_data = bytearray(self._read_inode(dir_inode))
    struct.pack_into("<I", inode_data, 124, entry_count + 1)

    checksum = crc32_with_hole(bytes(inode_data), 252, 4)
    struct.pack_into("<I", inode_data, 252, checksum)

    self._write_inode(dir_inode, bytes(inode_data))

  def _extract_leaf_entries(self, leaf_data, entry_count):
    """Extract directory entries from a leaf node."""
    entries = []
    offset = 16  # skip header

    for _ in range(entry_count):
      if offset >= BLOCK_SIZE:
        break

      record_len = struct.unpack_from("<H", leaf_data, offset + 8)[0]

      if record_len == 0 or offset + record_len > BLOCK_SIZE:
        break

      entries.append(bytes(leaf_data[offset:offset + record_len]))
      offset += record_len

    return entries

  def _find_in_directory(self, dir_inode, name):
    """Search a directory for an entry by name. Returns inode or None."""
    inode_data = self._read_inode(dir_inode)
    btree_root = struct.unpack_from("<I", inode_data, 120)[0]

    if btree_root == 0:
      return None

    name_bytes = name.encode("utf-8")
    target_hash = fnv1a_hash(name_bytes)

    return self._btree_search(btree_root, target_hash, name_bytes)

  def _btree_search(self, block_num, target_hash, name_bytes):
    """Search a B-tree for an entry by hash and name."""
    node_data = self._read_block(block_num)
    level, entry_count = struct.unpack_from("<HH", node_data, 0)

    if level == 0:
      # Leaf node — scan entries
      offset = 16

      for _ in range(entry_count):
        if offset >= BLOCK_SIZE:
          break

        inode_num, name_hash, record_len, name_len = \
          struct.unpack_from("<IIHB", node_data, offset)

        if record_len == 0:
          break

        if name_hash == target_hash and name_len == len(name_bytes):
          entry_name = node_data[offset + 12:offset + 12 + name_len]

          # Case-insensitive compare
          if entry_name.lower() == name_bytes.lower():
            return inode_num

        offset += record_len

      return None
    else:
      # Internal node — find correct child
      offset = 16

      # Read child pointers
      children = []

      for _ in range(entry_count):
        child_hash, child_block = struct.unpack_from("<II", node_data, offset)
        children.append((child_hash, child_block))
        offset += 8

      # Find the right child (last one whose hash <= target_hash)
      chosen_child = children[0][1]

      for child_hash, child_block in children:
        if child_hash <= target_hash:
          chosen_child = child_block
        else:
          break

      return self._btree_search(chosen_child, target_hash, name_bytes)

  def _read_inode(self, inode_number):
    """Read an inode from the inode table."""
    group_index = inode_number // self.inodes_per_group
    local_index = inode_number % self.inodes_per_group
    group = self.group_descriptors[group_index]
    table_block = group["inode_table"]
    inodes_per_block = BLOCK_SIZE // INODE_SIZE
    block_offset = local_index // inodes_per_block
    offset_in_block = (local_index % inodes_per_block) * INODE_SIZE
    absolute_offset = \
      (table_block + block_offset) * BLOCK_SIZE + offset_in_block

    return bytes(self.data[absolute_offset:absolute_offset + INODE_SIZE])

  def _increment_link_count(self, inode_number):
    """Increment an inode's link count."""
    inode_data = bytearray(self._read_inode(inode_number))
    link_count = struct.unpack_from("<H", inode_data, 2)[0]
    struct.pack_into("<H", inode_data, 2, link_count + 1)

    checksum = crc32_with_hole(bytes(inode_data), 252, 4)
    struct.pack_into("<I", inode_data, 252, checksum)

    self._write_inode(inode_number, bytes(inode_data))

  def _blocks_to_extents(self, blocks):
    """Convert a list of block numbers to extents (contiguous runs)."""
    if not blocks:
      return []

    extents = []
    run_start = blocks[0]
    run_logical = 0
    run_length = 1

    for i in range(1, len(blocks)):
      if blocks[i] == run_start + run_length:
        run_length += 1
      else:
        extents.append((run_logical, run_start, run_length))
        run_logical += run_length
        run_start = blocks[i]
        run_length = 1

    extents.append((run_logical, run_start, run_length))

    return extents

  def finalize(self):
    """Write all metadata structures and finalize the image."""
    # Write block group descriptor table
    for i, group in enumerate(self.group_descriptors):
      desc = bytearray(64)
      struct.pack_into("<IIIIIII", desc, 0,
                       group["block_bitmap"],
                       group["inode_bitmap"],
                       group["inode_table"],
                       group["free_blocks"],
                       group["free_inodes"],
                       group["directory_count"],
                       0)  # Flags

      desc_checksum = crc32_with_hole(bytes(desc), 28, 4)
      struct.pack_into("<I", desc, 28, desc_checksum)

      bgdt_offset = self.bgdt_start * BLOCK_SIZE + i * 64
      self.data[bgdt_offset:bgdt_offset + 64] = desc

    # Write block bitmaps and inode bitmaps
    for i, group in enumerate(self.group_descriptors):
      self._write_block(group["block_bitmap"], bytes(self.block_bitmaps[i]))
      self._write_block(group["inode_bitmap"], bytes(self.inode_bitmaps[i]))

    # Write journal superblock
    journal_sb = bytearray(BLOCK_SIZE)
    journal_sb[0:4] = JOURNAL_MAGIC
    struct.pack_into("<IIIIIII", journal_sb, 4,
                     0,                    # SequenceNumber
                     1,                    # HeadBlock (after journal SB)
                     1,                    # TailBlock (empty journal)
                     self.journal_blocks,  # BlockCount
                     BLOCK_SIZE,           # BlockSize
                     self.journal_blocks // 4,  # MaxTransactionBlocks
                     0)                    # Checksum (filled below)

    journal_checksum = crc32_with_hole(bytes(journal_sb), 28, 4)
    struct.pack_into("<I", journal_sb, 28, journal_checksum)

    self._write_block(self.journal_start, bytes(journal_sb))

    # Write superblock
    sb = bytearray(BLOCK_SIZE)
    sb[0:8] = SUPERBLOCK_MAGIC
    struct.pack_into("<HH", sb, 8, FormatVersionMajor, FormatVersionMinor)
    struct.pack_into("<I", sb, 12, BLOCK_SIZE_LOG2)
    struct.pack_into("<IIII", sb, 16,
                     self.total_blocks, self.free_blocks,
                     self.total_inodes, self.free_inodes)
    struct.pack_into("<I", sb, 32, ROOT_INODE)
    struct.pack_into("<II", sb, 36,
                     self.journal_start, self.journal_blocks)
    struct.pack_into("<III", sb, 44,
                     len(self.group_descriptors),
                     self.blocks_per_group,
                     self.inodes_per_group)
    struct.pack_into("<I", sb, 56, INODE_SIZE)
    struct.pack_into("<I", sb, 60, self.first_data_block)
    struct.pack_into("<I", sb, 64,
                     FEAT_JOURNAL | FEAT_EXTENTS |
                     FEAT_DIR_BTREE | FEAT_META_CSUM)
    struct.pack_into("<III", sb, 68, 0, 0, 0)  # compat flags
    sb[80:96] = self.volume_uuid

    label_bytes = self.label.encode("utf-8")[:31] + b"\x00"
    label_bytes += b"\x00" * (32 - len(label_bytes))
    sb[96:128] = label_bytes

    struct.pack_into("<II", sb, 128, 0, 0)  # MountCount, MaxMountCount
    struct.pack_into("<I", sb, 136, STATE_CLEAN)
    struct.pack_into("<III", sb, 140, 0, 0, self.now)  # timestamps
    struct.pack_into("<I", sb, 152, CSUM_CRC32)

    sb_checksum = crc32_with_hole(bytes(sb), 156, 4)
    struct.pack_into("<I", sb, 156, sb_checksum)

    self._write_block(1, bytes(sb))
    self._write_block(2, bytes(sb))  # backup

  def write(self, path):
    """Write the image to a file."""
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)

    with open(path, "wb") as f:
      f.write(self.data)


# Needed for version constants
FormatVersionMajor = 1
FormatVersionMinor = 0


# -- Directory Tree Import ----------------------------------------------------

def add_directory_tree(image, host_dir, qfs_prefix=""):
  """Recursively add files from a host directory to the QFS image."""
  for entry in sorted(os.listdir(host_dir)):
    host_path = os.path.join(host_dir, entry)
    qfs_path = f"{qfs_prefix}/{entry}" if qfs_prefix else entry

    if os.path.isfile(host_path):
      with open(host_path, "rb") as f:
        data = f.read()

      image.add_file(qfs_path, data)
    elif os.path.isdir(host_path):
      add_directory_tree(image, host_path, qfs_path)


# -- Main ----------------------------------------------------------------------

def main():
  parser = argparse.ArgumentParser(
    description="Create a QFS (Quantum File System) image."
  )
  parser.add_argument("-o", "--output", required=True,
                      help="Output image file path")
  parser.add_argument("-s", "--size", type=int, required=True,
                      help="Image size in megabytes")
  parser.add_argument("--label", default="System",
                      help="Volume label (default: System)")
  parser.add_argument("--from-dir",
                      help="Populate from a host directory tree")
  parser.add_argument("--empty", action="store_true",
                      help="Create an empty formatted image")

  args = parser.parse_args()

  size_bytes = args.size * 1024 * 1024
  total_blocks = size_bytes // BLOCK_SIZE

  if total_blocks < 64:
    print("Error: image too small (minimum ~256KB)", file=sys.stderr)
    sys.exit(1)

  image = QFSImage(total_blocks, label=args.label)

  # Create root directory
  image._create_root_directory()

  # Populate from directory if requested
  if args.from_dir:
    if not os.path.isdir(args.from_dir):
      print(f"Error: '{args.from_dir}' is not a directory", file=sys.stderr)
      sys.exit(1)

    add_directory_tree(image, args.from_dir)

  # Finalize metadata
  image.finalize()

  # Write output
  image.write(args.output)

  used_blocks = total_blocks - image.free_blocks
  used_inodes = image.total_inodes - image.free_inodes

  print(f"Created QFS image: {args.output}")
  print(f"  Label:       {args.label}")
  print(f"  Size:        {args.size} MB ({total_blocks} blocks)")
  print(f"  Groups:      {len(image.group_descriptors)}")
  print(f"  IndexNodes:      {used_inodes} / {image.total_inodes} used")
  print(f"  Blocks:      {used_blocks} / {total_blocks} used")
  print(f"  Journal:     {image.journal_blocks} blocks")
  print(f"  Free:        {image.free_blocks} blocks"
        f" ({image.free_blocks * BLOCK_SIZE // 1024} KB)")


if __name__ == "__main__":
  main()
