/**
 * @file Include/Quantum/Executable/QXF.hpp
 * @brief Quantum Executable Format (QXF) header structures and utilities.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Executable {
  /**
   * @brief QXF magic bytes: `"QXF\0"`.
   */
  inline constexpr UInt8 QXFMagic[4] = { 'Q', 'X', 'F', '\0' };

  /**
   * @brief Current QXF format version.
   */
  inline constexpr UInt16 QXFVersion = 1;

  /**
   * @brief QXF executable types.
   */
  enum class QXFType : UInt8 {
    /**
     * @brief Generic executable (`.qx`). Behaviour determined by flags.
     */
    Generic = 1,

    /**
     * @brief GUI application (`.qapp`). May bundle icons and resources.
     */
    Application = 2
  };

  /**
   * @brief QXF header flags.
   */
  namespace QXFFlags {
    /**
     * @brief Executable will create windows (compositor hint).
     */
    inline constexpr UInt8 NeedsDisplay = 1 << 0;

    /**
     * @brief Executable requests elevated privilege.
     */
    inline constexpr UInt8 Privileged = 1 << 1;
  }

  /**
   * @brief QXF file header (32 bytes, packed).
   *
   * All offsets are relative to the start of the file.
   */
  struct QXFHeader {
    /**
     * @brief Magic bytes, must be @ref QXFMagic (`"QXF\0"`).
     */
    UInt8 Magic[4];

    /**
     * @brief Format version.
     */
    UInt16 Version;

    /**
     * @brief Executable type.
     */
    QXFType Type;

    /**
     * @brief Flags bitfield. See @ref QXFFlags.
     */
    UInt8 Flags;

    /**
     * @brief Byte offset from file start to the ELF32 payload.
     */
    UInt32 ELFOffset;

    /**
     * @brief Size of the ELF32 payload in bytes.
     */
    UInt32 ELFSize;

    /**
     * @brief Byte offset to the metadata section, or `0` if none.
     */
    UInt32 MetadataOffset;

    /**
     * @brief Size of the metadata section in bytes.
     */
    UInt32 MetadataSize;

    /**
     * @brief Byte offset to the resource table, or `0` if none.
     */
    UInt32 ResourceTableOffset;

    /**
     * @brief Number of entries in the resource table.
     */
    UInt16 ResourceCount;

    /**
     * @brief Reserved, must be `0`.
     */
    UInt16 Reserved;
  } __attribute__((packed));

  /**
   * @brief QXF metadata key identifiers.
   */
  enum class QXFMetadataKey : UInt16 {
    /**
     * @brief Human-readable display name (string).
     */
    DisplayName = 1,

    /**
     * @brief Semantic version string (string).
     */
    Version = 2,

    /**
     * @brief Author or vendor name (string).
     */
    Author = 3,

    /**
     * @brief Application category (@ref UInt16).
     */
    Category = 4,

    /**
     * @brief One-line description (string).
     */
    Description = 5,

    /**
     * @brief Short usage string for `help` output (string).
     */
    UsageSummary = 6,

    /**
     * @brief Minimum required OS version (@ref UInt32).
     */
    MinOSVersion = 7
  };

  /**
   * @brief A single metadata entry header in the metadata section.
   *
   * Entries are packed consecutively, each padded to 4-byte alignment.
   * The value data immediately follows this header.
   */
  struct QXFMetadataEntry {
    /**
     * @brief Metadata key identifier.
     */
    QXFMetadataKey Key;

    /**
     * @brief Length of the value data in bytes.
     */
    UInt16 Length;
  } __attribute__((packed));

  /**
   * @brief QXF resource type identifiers.
   */
  enum class QXFResourceType : UInt16 {
    /**
     * @brief Icon resource. SubType indicates size:
     *        `1`=16x16, `2`=32x32, `3`=48x48, `4`=64x64.
     */
    Icon = 1,

    /**
     * @brief Bitmap resource. SubType `0` = raw BGRA pixel data.
     */
    Bitmap = 2,

    /**
     * @brief String resource. SubType = locale ID (`0` = default).
     */
    String = 3,

    /**
     * @brief Font resource. SubType `0` = default.
     */
    Font = 4,

    /**
     * @brief Application-defined raw resource.
     */
    Raw = 5,

    /**
     * @brief QLL import table. Declares dynamic library dependencies and
     *        the GOT slots to patch at load time.
     */
    ImportTable = 6
  };

  /**
   * @brief Maximum length of a resource name, including null terminator.
   */
  inline constexpr Size QXFResourceNameMaxLength = 32;

  /**
   * @brief A single resource table entry (44 bytes, packed).
   */
  struct QXFResourceEntry {
    /**
     * @brief Resource type.
     */
    QXFResourceType Type;

    /**
     * @brief Type-specific qualifier.
     */
    UInt16 SubType;

    /**
     * @brief Byte offset from file start to the resource data.
     */
    UInt32 DataOffset;

    /**
     * @brief Size of the resource data in bytes.
     */
    UInt32 DataSize;

    /**
     * @brief Null-terminated resource name.
     */
    char Name[QXFResourceNameMaxLength];
  } __attribute__((packed));

  /**
   * @brief Header for the import table resource data.
   */
  struct QXFImportTableHeader {
    /**
     * @brief Number of QLL dependencies.
     */
    UInt16 DependencyCount;

    /**
     * @brief Reserved, must be `0`.
     */
    UInt16 Reserved;
  } __attribute__((packed));

  /**
   * @brief A single QLL dependency declaration in the import table.
   *
   * The loader resolves this by searching for a QLL whose
   * @ref Quantum::Executable::QLLIdentity::Name matches @ref Name and
   * whose version satisfies @ref MinVersion. Architecture and platform
   * are checked implicitly (the QLL must match the running system).
   *
   * Immediately followed by @ref SymbolCount @ref QXFImportSymbol
   * entries, which identify the GOT slots to patch.
   */
  struct QXFImportDependency {
    /**
     * @brief Canonical library name (e.g. `"Quantum.Core"`),
     *        null-terminated. Must match the QLL identity's
     *        @ref Quantum::Executable::QLLIdentity::Name.
     */
    char Name[64];

    /**
     * @brief Minimum library version required. The loader rejects
     *        a QLL whose version is below this threshold.
     */
    UInt32 MinVersion;

    /**
     * @brief Number of symbols imported from this library.
     */
    UInt32 SymbolCount;
  } __attribute__((packed));

  /**
   * @brief A single imported symbol within a dependency entry.
   */
  struct QXFImportSymbol {
    /**
     * @brief Hash of the imported symbol name (djb2).
     */
    UInt32 NameHash;

    /**
     * @brief Relative offset into the executable's `.got` section where
     *        the resolved address should be written.
     */
    UInt32 GOTSlotRVA;
  } __attribute__((packed));

  /**
   * @brief Validates a QXF file header.
   * @param data Pointer to the file data.
   * @param size Size of the file data in bytes.
   * @return `true` if the header is valid; `false` otherwise.
   */
  inline bool ValidateQXF(const UInt8* data, Size size) {
    if (!data || size < sizeof(QXFHeader)) return false;

    auto* header = reinterpret_cast<const QXFHeader*>(data);

    // check magic
    for (UInt32 i = 0; i < 4; ++i) {
      if (header->Magic[i] != QXFMagic[i]) return false;
    }

    // check version
    if (header->Version == 0 || header->Version > QXFVersion) return false;

    // check type
    if (
      header->Type != QXFType::Generic &&
      header->Type != QXFType::Application
    ) return false;

    // check ELF bounds
    if (header->ELFOffset < sizeof(QXFHeader)) return false;

    Size elfEnd
      = static_cast<Size>(header->ELFOffset)
      + static_cast<Size>(header->ELFSize);

    if (elfEnd < header->ELFOffset || elfEnd > size) return false;

    // check metadata bounds
    if (header->MetadataOffset != 0) {
      Size metadataEnd
        = static_cast<Size>(header->MetadataOffset)
        + static_cast<Size>(header->MetadataSize);

      if (metadataEnd < header->MetadataOffset || metadataEnd > size) {
        return false;
      }
    }

    // check resource table bounds
    if (header->ResourceTableOffset != 0) {
      Size tableSize
        = static_cast<Size>(header->ResourceCount)
        * sizeof(QXFResourceEntry);

      Size tableEnd
        = static_cast<Size>(header->ResourceTableOffset)
        + tableSize;

      if (tableEnd < header->ResourceTableOffset || tableEnd > size) {
        return false;
      }
    }

    // check reserved
    if (header->Reserved != 0) return false;

    return true;
  }

  /**
   * @brief Checks whether a file starts with the ELF magic bytes.
   * @param data Pointer to the file data.
   * @param size Size of the file data in bytes.
   * @return `true` if the first 4 bytes are `\x7FELF`.
   */
  inline bool IsELF(const UInt8* data, Size size) {
    if (!data || size < 4) return false;

    return data[0] == 0x7F
      && data[1] == 'E'
      && data[2] == 'L'
      && data[3] == 'F';
  }

  /**
   * @brief Checks whether a file starts with the QXF magic bytes.
   * @param data Pointer to the file data.
   * @param size Size of the file data in bytes.
   * @return `true` if the first 4 bytes are `"QXF\0"`.
   */
  inline bool IsQXF(const UInt8* data, Size size) {
    if (!data || size < 4) return false;

    return data[0] == 'Q'
      && data[1] == 'X'
      && data[2] == 'F'
      && data[3] == '\0';
  }
}
