/**
 * @file Servers/Run/Core/ELF32/ELF32.hpp
 * @brief Declares and implements @ref @QRunSrv::ELF32::ValidateELF32Header.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <RunServerTypes.hpp>

#include "ELF32Header.hpp"

namespace Quantum::Servers::Run::Core::ELF32 {
  /**
   * @brief Validates an ELF32 image header.
   * @param image Pointer to the image data.
   * @param size Size of the image in bytes.
   * @return `true` if the image has a valid ELF32 header; `false` otherwise.
   */
  inline bool ValidateELF32Header(const UInt8* image, Size size) {
    if (
      !image ||
      size < sizeof(ELF32Header)
    ) {
      return false;
    }

    const ELF32Header* header = Cast::As<ELF32Header>(image);

    // check magic bytes
    for (UInt32 i = 0; i < 4; ++i) {
      if (header->Identifier[i] != ELF32Magic[i]) {
        return false;
      }
    }

    // check class, data encoding, version
    if (
      header->Identifier[ELF32IdentifierClass] != ELF32Class ||
      header->Identifier[ELF32IdentifierData] != ELF32Data2LSB ||
      header->Identifier[ELF32IdentifierVersion] != ELF32VersionCurrent ||
      header->ProgramHeaderEntrySizeInBytes < sizeof(ELF32ProgramHeader)
    ) {
      return false;
    }

    // check that program headers fit within image (with overflow checks)
    Size programHeaderBytes
      = static_cast<Size>(header->ProgramHeaderEntrySizeInBytes)
      * static_cast<Size>(header->ProgramHeaderCount);

    // detect multiplication overflow: if the result is smaller than either
    // operand when both are non-zero, the multiplication wrapped
    if (
      header->ProgramHeaderCount != 0 &&
      programHeaderBytes / header->ProgramHeaderCount
        != header->ProgramHeaderEntrySizeInBytes
    ) {
      return false;
    }

    Size programHeaderEnd
      = static_cast<Size>(header->ProgramHeaderOffset)
      + programHeaderBytes;

    // detect addition overflow
    if (programHeaderEnd < static_cast<Size>(header->ProgramHeaderOffset)) {
      return false;
    }

    if (
      static_cast<Size>(header->ProgramHeaderOffset) >= size ||
      programHeaderEnd > size
    ) {
      return false;
    }

    return true;
  }
}
