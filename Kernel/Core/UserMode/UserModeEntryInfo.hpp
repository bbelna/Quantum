/**
 * @file Kernel/UserMode/UserModeEntryInfo.hpp
 * @brief Declares @ref @QKrnl::UserMode::UserModeEntryInfo.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::UserMode {
  /**
   * @brief User mode entry information.
   *
   * Contains all information needed to enter user mode for the first time.
   */
  struct UserModeEntryInfo {
    /**
     * @brief User-mode instruction pointer (entry point).
     */
    UIntPtr EntryPoint;

    /**
     * @brief User-mode stack pointer.
     */
    UIntPtr StackPointer;
  };
}
