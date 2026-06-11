/**
 * @file Servers/Run/Core/ELF32/GetELF32ProgramHeader.hpp
 * @brief Declares and implements @ref @QRunSrv::ELF32::GetELF32ProgramHeader.
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
   * @brief Gets the program header at the given index.
   * @param image Pointer to the ELF image.
   * @param index Program header index.
   * @return Pointer to the program header.
   */
  inline const ELF32ProgramHeader* GetELF32ProgramHeader(
    const UInt8* image,
    UInt32 index
  ) {
    const ELF32Header* header = Cast::As<ELF32Header>(image);

    return Cast::As<ELF32ProgramHeader>(
      image
        + header->ProgramHeaderOffset
        + index * header->ProgramHeaderEntrySizeInBytes
    );
  }
}
