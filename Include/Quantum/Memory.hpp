/**
 * @file Include/Quantum/Memory.hpp
 * @brief Top-level memory allocation helpers for Quantum userspace.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Kernel/ABI/Memory.hpp>
#include <Quantum/Kernel/Memory/MemoryTypes.hpp>
#include <Quantum/Types.hpp>

/**
  * @brief Allocates @p sizeInBytes bytes of memory in the calling process'
  *        address space.
  * @param sizeInBytes Number of bytes to allocate.
  * @return Address of the allocated block.
  */
inline UIntPtr AllocateBlock(Size sizeInBytes) {
  return Quantum::Kernel::Memory::ABI::Allocate(sizeInBytes);
}

/**
  * @brief Frees the block of memory at @p address, which must have been
  *        returned by a previous call to @ref AllocateBlock.
  * @param address Address of the block to free.
  */
inline void FreeBlock(UIntPtr address) {
  Quantum::Kernel::Memory::ABI::Free(address);
}

/**
  * @brief Frees the block of memory at @p address, which must have been
  *        returned by a previous call to @ref AllocateBlock.
  * @tparam AddressType Pointer type of the block to free.
  * @param address Address of the block to free.
  */
template <typename AddressType>
inline void FreeBlock(AddressType address) {
  FreeBlock(reinterpret_cast<UIntPtr>(address));
}

/**
  * @brief Creates a shared buffer of the specified size that can be shared
  *        between processes.
  * @param sizeInBytes The size of the buffer in bytes.
  * @return The @ref SharedBufferID on success, or `0` on failure.
  */
inline SharedBufferID CreateShared(Size sizeInBytes) {
  return Quantum::Kernel::Memory::ABI::CreateShared(sizeInBytes);
}

/**
  * @brief Maps an existing shared buffer into the caller's address space.
  * @param id The @ref SharedBufferID returned by @ref CreateShared.
  * @return The address of the mapping, or `0` on failure.
  */
inline UIntPtr AttachShared(SharedBufferID id) {
  return Quantum::Kernel::Memory::ABI::AttachShared(id);
}

/**
 * @brief Unmaps a shared buffer from the caller's address space.
 * @param address The virtual address of the attached mapping.
 */
inline void DetachShared(UIntPtr address) {
  return Quantum::Kernel::Memory::ABI::DetachShared(address);
}
