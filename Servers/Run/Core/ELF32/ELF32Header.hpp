/**
 * @file Servers/Run/Core/ELF32/ELF32Header.hpp
 * @brief Declares @ref @QRunSrv::ELF32::ELF32Header.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <RunServerTypes.hpp>

#include "ELF32Constants.hpp"
#include "ELF32ProgramHeader.hpp"

namespace Quantum::Servers::Run::Core::ELF32 {
  /**
   * @brief ELF32 file header.
   */
  struct ELF32Header {
    /**
     * @brief ELF identification bytes.
     *
     * The first 4 bytes must be the ELF magic numbers, followed by class, data
     * encoding, version, and padding.
     */
    UInt8 Identifier[16];

    /**
     * @brief Object file type (e.g., executable, shared object, etc.).
     */
    UInt16 FileType;

    /**
     * @brief Machine architecture (e.g., x86, ARM, etc.).
     */
    UInt16 Machine;

    /**
     * @brief ELF version.
     */
    UInt32 Version;

    /**
     * @brief Entry point virtual address.
     */
    UInt32 EntryAddress;

    /**
     * @brief Program header table file offset.
     */
    UInt32 ProgramHeaderOffset;

    /**
     * @brief Section header table file offset.
     */
    UInt32 SectionHeaderOffset;

    /**
     * @brief Processor-specific flags.
     */
    UInt32 Flags;

    /**
     * @brief ELF header size in bytes.
     */
    UInt16 HeaderSizeInBytes;

    /**
     * @brief Size of each program header entry in bytes.
     */
    UInt16 ProgramHeaderEntrySizeInBytes;

    /**
     * @brief Number of program header entries.
     */
    UInt16 ProgramHeaderCount;

    /**
     * @brief Size of each section header entry in bytes.
     */
    UInt16 SectionHeaderEntrySizeInBytes;

    /**
     * @brief Number of section header entries.
     */
    UInt16 SectionHeaderCount;

    /**
     * @brief Section header string table index.
     */
    UInt16 SectionHeaderStringIndex;
  };
}
