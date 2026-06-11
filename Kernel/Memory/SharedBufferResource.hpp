/**
 * @file Kernel/Memory/SharedBufferResource.hpp
 * @brief Declares @ref @QKrnl::Memory::SharedBufferResource.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "SharedBufferRights.hpp"
#include "Resources/KernelResource.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Capability-style resource handle for @ref SharedBuffer.
   *
   * Wraps a @ref KernelResource pairing an ID (@ref SharedBufferID) with a
   * set of @ref SharedBufferRights that describe the holder's relationship to
   * the associated @ref SharedBuffer.
   */
  struct SharedBufferResource : public KernelResource<
    SharedBufferID,
    SharedBufferRights
  > {};
}
