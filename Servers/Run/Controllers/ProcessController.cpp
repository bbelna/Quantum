/**
 * @file Servers/Run/Controllers/ProcessController.cpp
 * @brief Implements @ref @QRunSrv::ProcessController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ProcessController.hpp"

namespace Quantum::Servers::Run::Controllers {
  ProcessController::ProcessController(
    KernelClient& kernel,
    ServerLog& log,
    ProcessTable& processTable
  ) :
    RequestController(
      kernel,
      log,
      Route<&ProcessController::_handleSetWorkingDirectory>(
        RunServerOperation::SetWorkingDirectory
      ),
      Route<&ProcessController::_handleGetWorkingDirectory>(
        RunServerOperation::GetWorkingDirectory
      ),
      Route<&ProcessController::_handleGetProgramDirectory>(
        RunServerOperation::GetProgramDirectory
      )
    ),
    _processTable(processTable)
  {
  }

  void ProcessController::_handleSetWorkingDirectory(
    const SetWorkingDirectoryRequest& request
  ) {
    ProcessEntry* entry = _processTable.FindOrCreate(request.PID);

    if (entry) {
      CString::Copy(
        request.Path,
        entry->WorkingDirectory,
        sizeof(entry->WorkingDirectory)
      );

      _log.Trace(
        "PID %u cwd = \"%s\"",
        request.PID,
        entry->WorkingDirectory
      );
    } else {
      _log.Warning("SetWorkingDirectory: no free process slots");
    }
  }

  void ProcessController::_handleGetWorkingDirectory(
    const GetWorkingDirectoryRequest& request
  ) {
    GetWorkingDirectoryResult result = {};
    ProcessEntry* entry = _processTable.Find(request.PID);

    if (entry) {
      result.Success = true;

      CString::Copy(
        entry->WorkingDirectory,
        result.Path,
        sizeof(result.Path)
      );
    }

    SendReply(
      request.ReplyPortID,
      &result,
      sizeof(result)
    );
  }

  void ProcessController::_handleGetProgramDirectory(
    const GetProgramDirectoryRequest& request
  ) {
    GetProgramDirectoryResult result = {};
    ProcessEntry* entry = _processTable.Find(request.PID);

    if (entry) {
      result.Success = true;

      CString::Copy(
        entry->ProgramDirectory,
        result.Path,
        sizeof(result.Path)
      );
    }

    SendReply(
      request.ReplyPortID,
      &result,
      sizeof(result)
    );
  }
}
