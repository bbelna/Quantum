/**
 * @file Servers/Run/Core/Process/ProcessTable.cpp
 * @brief Implements @ref @QRunSrv::Process::ProcessTable.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ProcessTable.hpp"

namespace Quantum::Servers::Run::Core::Process {
  ProcessEntry* ProcessTable::FindOrCreate(ProcessID pid) {
    // first pass: find existing entry
    for (Size index = 0; index < MaxProcessEntries; ++index) {
      if (_entries[index].PID == pid) {
        return &_entries[index];
      }
    }

    // second pass: find an unused slot
    for (Size index = 0; index < MaxProcessEntries; ++index) {
      if (_entries[index].PID == 0) {
        _entries[index].PID = pid;
        _entries[index].WorkingDirectory[0] = '\0';

        return &_entries[index];
      }
    }

    return nullptr;
  }

  ProcessEntry* ProcessTable::Find(ProcessID pid) {
    for (Size index = 0; index < MaxProcessEntries; ++index) {
      if (_entries[index].PID == pid) {
        return &_entries[index];
      }
    }

    return nullptr;
  }
}
