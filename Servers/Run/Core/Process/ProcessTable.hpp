/**
 * @file Servers/Run/Core/Process/ProcessTable.hpp
 * @brief Declares @ref @QRunSrv::Process::ProcessTable.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "ProcessConstants.hpp"
#include "ProcessEntry.hpp"

namespace Quantum::Servers::Run::Core::Process {
  /**
   * @brief Fixed-size table of per-process metadata entries.
   */
  class ProcessTable {
    public:
      /**
       * @brief Finds or creates a @ref ProcessEntry for the given
       *        @ref ProcessID.
       * @param pid The @ref ProcessID to find or allocate a slot for.
       * @return Pointer to the @ref ProcessEntry, or `nullptr` if the table is
       *         full.
       */
      ProcessEntry* FindOrCreate(ProcessID pid);

      /**
       * @brief Finds a @ref ProcessEntry by @ref ProcessID.
       * @param pid The @ref ProcessID to search for.
       * @return Pointer to the @ref ProcessEntry, or `nullptr` if not found.
       */
      ProcessEntry* Find(ProcessID pid);

    private:
      /**
       * @brief @ref ProcessEntry table.
       */
      ProcessEntry _entries[MaxProcessEntries] = {};
  };
}
