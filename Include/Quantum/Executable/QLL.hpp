/**
 * @file Include/Quantum/Executable/QLL.hpp
 * @brief Quantum Loadable Library (QLL) header structures and utilities.
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
   * @brief QLL magic bytes: `"QLL\0"`.
   */
  inline constexpr UInt8 QLLMagic[4] = { 'Q', 'L', 'L', '\0' };

  /**
   * @brief Current QLL format version.
   */
  inline constexpr UInt16 QLLVersion = 1;

  /**
   * @brief Maximum length of a QLL library name, including null terminator.
   */
  inline constexpr Size QLLNameMaxLength = 64;

  /**
   * @brief Maximum length of a QLL architecture identifier, including null
   *        terminator.
   */
  inline constexpr Size QLLArchMaxLength = 16;

  /**
   * @brief Maximum length of a QLL platform identifier, including null
   *        terminator.
   */
  inline constexpr Size QLLPlatformMaxLength = 16;

  /**
   * @brief QLL header flags.
   */
  namespace QLLFlags {
    /**
     * @brief Library contains global constructors requiring initialization.
     */
    inline constexpr UInt16 HasInit = 1 << 0;
  }

  /**
   * @brief Packed semantic version (major.minor.patch).
   *
   * Encoded as a single `UInt32`: bits 31..22 = major (10 bits, 0..1023),
   * bits 21..12 = minor (10 bits), bits 11..0 = patch (12 bits, 0..4095).
   */
  struct QLLVersion32 {
    UInt32 Packed;

    /**
     * @brief Constructs a version from individual components.
     * @param major Major version number (0..1023).
     * @param minor Minor version number (0..1023).
     * @param patch Patch version number (0..4095).
     */
    static constexpr QLLVersion32 Make(
      UInt16 major,
      UInt16 minor,
      UInt16 patch
    ) {
      return QLLVersion32 {
        (static_cast<UInt32>(major & 0x3FF) << 22)
        | (static_cast<UInt32>(minor & 0x3FF) << 12)
        | static_cast<UInt32>(patch & 0xFFF)
      };
    }

    /**
     * @brief Returns the major version number.
     */
    constexpr UInt16 Major() const {
      return static_cast<UInt16>((Packed >> 22) & 0x3FF);
    }

    /**
     * @brief Returns the minor version number.
     */
    constexpr UInt16 Minor() const {
      return static_cast<UInt16>((Packed >> 12) & 0x3FF);
    }

    /**
     * @brief Returns the patch version number.
     */
    constexpr UInt16 Patch() const {
      return static_cast<UInt16>(Packed & 0xFFF);
    }

    /**
     * @brief Tests whether this version satisfies a minimum requirement.
     * @param minimum The minimum version to compare against.
     * @return `true` if this version is greater than or equal to
     *         @p minimum.
     */
    constexpr bool Satisfies(QLLVersion32 minimum) const {
      return Packed >= minimum.Packed;
    }
  } __attribute__((packed));

  /**
   * @brief Library identity block embedded in the QLL header.
   *
   * Uniquely identifies a library by name, version, and target triple.
   * The loader uses this to verify that a QLL on disk matches what an
   * executable requested.
   */
  struct QLLIdentity {
    /**
     * @brief Canonical library name (e.g. `"Quantum.Core"`), null-terminated.
     */
    char Name[QLLNameMaxLength];

    /**
     * @brief Library version (semantic versioning, packed).
     */
    QLLVersion32 LibraryVersion;

    /**
     * @brief Target architecture identifier (e.g. `"IA32"`, `"x86_64"`),
     *        null-terminated.
     */
    char Architecture[QLLArchMaxLength];

    /**
     * @brief Target platform identifier (e.g. `"PC"`, `"RPi"`),
     *        null-terminated.
     */
    char Platform[QLLPlatformMaxLength];

    /**
     * @brief Minimum QuantumOS version required to load this library.
     */
    QLLVersion32 MinOSVersion;
  } __attribute__((packed));

  /**
   * @brief QLL file header (packed).
   *
   * All offsets are relative to the start of the file. The header
   * contains a full @ref QLLIdentity block so the loader can verify
   * name, version, and architecture compatibility before proceeding.
   */
  struct QLLHeader {
    /**
     * @brief Magic bytes, must be @ref QLLMagic (`"QLL\0"`).
     */
    UInt8 Magic[4];

    /**
     * @brief Format version (version of the QLL container format itself,
     *        not the library version).
     */
    UInt16 FormatVersion;

    /**
     * @brief Flags bitfield. See @ref QLLFlags.
     */
    UInt16 Flags;

    /**
     * @brief Library identity: name, version, architecture, platform.
     */
    QLLIdentity Identity;

    /**
     * @brief Byte offset from file start to the ELF32 shared object
     *        payload.
     */
    UInt32 ELFOffset;

    /**
     * @brief Size of the ELF32 payload in bytes.
     */
    UInt32 ELFSize;

    /**
     * @brief Byte offset to the export table, or `0` if none.
     */
    UInt32 ExportTableOffset;

    /**
     * @brief Number of entries in the export table.
     */
    UInt32 ExportCount;

    /**
     * @brief Byte offset to the string table, or `0` if none.
     */
    UInt32 StringTableOffset;

    /**
     * @brief Size of the string table in bytes.
     */
    UInt32 StringTableSize;

    /**
     * @brief Byte offset to the dependency table, or `0` if none.
     *        Lists other QLLs that this library depends on.
     */
    UInt32 DependencyTableOffset;

    /**
     * @brief Number of entries in the dependency table.
     */
    UInt16 DependencyCount;

    /**
     * @brief Reserved, must be `0`.
     */
    UInt16 Reserved;
  } __attribute__((packed));

  /**
   * @brief QLL export symbol flags.
   */
  namespace QLLExportFlags {
    /**
     * @brief Symbol is a function entry point.
     */
    inline constexpr UInt16 Function = 1 << 0;
  }

  /**
   * @brief A single export table entry (12 bytes, packed).
   */
  struct QLLExportEntry {
    /**
     * @brief Byte offset into the string table for the symbol name.
     */
    UInt32 NameOffset;

    /**
     * @brief Relative virtual address (offset from the ELF load base).
     *        Resolved to an absolute address at load time.
     */
    UInt32 Value;

    /**
     * @brief Symbol flags. See @ref QLLExportFlags.
     */
    UInt16 Flags;

    /**
     * @brief Reserved, must be `0`.
     */
    UInt16 Reserved;
  } __attribute__((packed));

  /**
   * @brief A dependency entry declaring that this QLL requires another QLL.
   */
  struct QLLDependencyEntry {
    /**
     * @brief Canonical name of the required library
     *        (e.g. `"Quantum.Threading"`), null-terminated.
     */
    char Name[QLLNameMaxLength];

    /**
     * @brief Minimum version of the required library.
     */
    QLLVersion32 MinVersion;
  } __attribute__((packed));

  /**
   * @brief Computes a djb2 hash of a null-terminated string.
   * @param name The string to hash.
   * @return The 32-bit hash value.
   */
  inline constexpr UInt32 QLLHashName(const char* name) {
    UInt32 hash = 5381;

    while (*name) {
      hash = ((hash << 5) + hash) + static_cast<UInt8>(*name);
      ++name;
    }

    return hash;
  }

  /**
   * @brief Validates a QLL file header.
   * @param data Pointer to the file data.
   * @param size Size of the file data in bytes.
   * @return `true` if the header is valid; `false` otherwise.
   */
  inline bool ValidateQLL(const UInt8* data, Size size) {
    if (!data || size < sizeof(QLLHeader)) return false;

    auto* header = reinterpret_cast<const QLLHeader*>(data);

    // check magic
    for (UInt32 i = 0; i < 4; ++i) {
      if (header->Magic[i] != QLLMagic[i]) return false;
    }

    // check format version
    if (
      header->FormatVersion == 0 ||
      header->FormatVersion > QLLVersion
    ) return false;

    // check identity: name must be non-empty and null-terminated
    if (header->Identity.Name[0] == '\0') return false;

    bool nameTerminated = false;

    for (Size i = 0; i < QLLNameMaxLength; ++i) {
      if (header->Identity.Name[i] == '\0') {
        nameTerminated = true;

        break;
      }
    }

    if (!nameTerminated) return false;

    // check ELF bounds
    if (header->ELFOffset < sizeof(QLLHeader)) return false;

    Size elfEnd
      = static_cast<Size>(header->ELFOffset)
      + static_cast<Size>(header->ELFSize);

    if (elfEnd < header->ELFOffset || elfEnd > size) return false;

    // check export table bounds
    if (header->ExportTableOffset != 0) {
      Size tableSize
        = static_cast<Size>(header->ExportCount)
        * sizeof(QLLExportEntry);

      Size tableEnd
        = static_cast<Size>(header->ExportTableOffset)
        + tableSize;

      if (tableEnd < header->ExportTableOffset || tableEnd > size) {
        return false;
      }
    }

    // check string table bounds
    if (header->StringTableOffset != 0) {
      Size stringEnd
        = static_cast<Size>(header->StringTableOffset)
        + static_cast<Size>(header->StringTableSize);

      if (stringEnd < header->StringTableOffset || stringEnd > size) {
        return false;
      }
    }

    // check dependency table bounds
    if (header->DependencyTableOffset != 0) {
      Size depSize
        = static_cast<Size>(header->DependencyCount)
        * sizeof(QLLDependencyEntry);

      Size depEnd
        = static_cast<Size>(header->DependencyTableOffset)
        + depSize;

      if (depEnd < header->DependencyTableOffset || depEnd > size) {
        return false;
      }
    }

    // check reserved
    if (header->Reserved != 0) return false;

    return true;
  }

  /**
   * @brief Checks whether a file starts with the QLL magic bytes.
   * @param data Pointer to the file data.
   * @param size Size of the file data in bytes.
   * @return `true` if the first 4 bytes are `"QLL\0"`.
   */
  inline bool IsQLL(const UInt8* data, Size size) {
    if (!data || size < 4) return false;

    return data[0] == 'Q'
      && data[1] == 'L'
      && data[2] == 'L'
      && data[3] == '\0';
  }
}
