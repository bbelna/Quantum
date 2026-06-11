/**
 * @file Servers/Run/Core/ELF32/ELF32.hpp
 * @brief Declares constants for @ref @QRunSrv::ELF32.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <RunServerTypes.hpp>

namespace Quantum::Servers::Run::Core::ELF32 {
  /**
   * @brief ELF magic bytes.
   */
  constexpr UInt8 ELF32Magic[4] = { 0x7F, 'E', 'L', 'F' };

  /**
   * @brief ELF identification index: class.
   */
  constexpr UInt32 ELF32IdentifierClass = 4;

  /**
   * @brief ELF identification index: data encoding.
   */
  constexpr UInt32 ELF32IdentifierData = 5;

  /**
   * @brief ELF identification index: version.
   */
  constexpr UInt32 ELF32IdentifierVersion = 6;

  /**
   * @brief ELF class: 32-bit.
   */
  constexpr UInt8 ELF32Class = 1;

  /**
   * @brief ELF data encoding: little-endian.
   */
  constexpr UInt8 ELF32Data2LSB = 1;

  /**
   * @brief ELF version: current.
   */
  constexpr UInt8 ELF32VersionCurrent = 1;

  /**
   * @brief Memory page size for alignment.
   *
   * TODO: This needs to be removed and provided by the kernel.
   */
  constexpr UInt32 BlockSize = 4096;

  /**
   * @brief Base address of the kernel.
   *
   * TODO: This needs to be removed and provided by the kernel.
   */
  constexpr UInt32 KernelBase = 0xC0000000;
}
