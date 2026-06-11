/**
 * @file Kernel/Structures/IntrusiveFreeNode.hpp
 * @brief Declares @ref @QKrnl::Core::FreeNode.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Kernel::Core {
  /**
    * @brief Intrusive free-list node.
    */
  struct IntrusiveFreeNode {
    /**
      * @brief Pointer to the next @ref IntrusiveFreeNode, or `nullptr` if
      *        this is the end of the free list.
      */
    IntrusiveFreeNode* Next;
  };
}
