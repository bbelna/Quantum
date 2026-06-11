/**
 * @file Servers/Run/Controllers/LoadController.cpp
 * @brief Implements @ref @QRunSrv::Controllers::LoadController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ELF32/ELF32Loader.hpp>

#include "LoadController.hpp"

namespace Quantum::Servers::Run::Controllers {
  LoadController::LoadController(
    KernelClient& kernel,
    ServerLog& log,
    ProcessTable& processTable
  ) :
    RequestController(
      kernel,
      log,
      Route<&LoadController::_handleLoadELF>(RunServerOperation::LoadELF)
    ),
    _processTable(processTable)
  {
  }

  void LoadController::_handleLoadELF(
    const LoadELFRequest& request,
    const IPCMessage* message
  ) {
    // binary data follows immediately after the LoadELFRequest struct;
    // argument data (if any) follows after the binary data
    const UInt8* payloadStart
      = reinterpret_cast<const UInt8*>(&request + 1);
    Size payloadSize
      = message->PayloadSizeInBytes
      - sizeof(LoadELFRequest);

    Size binarySize
      = request.ELFSize > 0
      ? request.ELFSize
      : payloadSize;

    const UInt8* binaryData = payloadStart;
    const UInt8* elfData = binaryData;
    Size elfSize = binarySize;

    // detect QXF container: extract the ELF payload from inside
    if (IsQXF(binaryData, binarySize)) {
      if (!ValidateQXF(binaryData, binarySize)) {
        _log.Error(
          "Invalid QXF header for %s",
          request.Name
        );

        return;
      }

      const QXFHeader* qxfHeader = Cast::As<QXFHeader>(binaryData);

      elfData = binaryData + qxfHeader->ELFOffset;
      elfSize = qxfHeader->ELFSize;
    }

    Size argumentCount = request.ArgumentCount;
    const char* argumentData = nullptr;
    Size argumentDataSize = 0;

    if (
      argumentCount > 0 &&
      binarySize < payloadSize
    ) {
      argumentData = reinterpret_cast<const char*>(payloadStart + binarySize);
      argumentDataSize = payloadSize - binarySize;
    }

    // extract inherited stream buffer IDs
    UInt8 streamCount = request.StreamCount;
    const SharedBufferID* streamBufferIDs = nullptr;

    if (streamCount > 0) {
      streamBufferIDs = request.StreamBufferIDs;
    }

    ProcessID pid = ELF32Loader::Load(
      request.Name,
      elfData,
      elfSize,
      argumentCount,
      argumentData,
      argumentDataSize,
      streamCount,
      streamBufferIDs
    );

    // register the new process with its cwd and program directory
    if (pid != InvalidProcessID) {
      ProcessEntry* childEntry = _processTable.FindOrCreate(pid);

      if (childEntry) {
        CString::Copy(
          request.WorkingDirectory,
          childEntry->WorkingDirectory,
          sizeof(childEntry->WorkingDirectory)
        );

        // derive the program directory from the binary path by
        // stripping the filename component
        const char* name = request.Name;
        Size lastSlash = 0;
        bool hasSlash = false;

        for (Size i = 0; name[i] != '\0'; ++i) {
          if (name[i] == '/') {
            lastSlash = i;
            hasSlash = true;
          }
        }

        if (hasSlash) {
          Size dirLength = lastSlash;

          if (dirLength >= sizeof(childEntry->ProgramDirectory)) {
            dirLength = sizeof(childEntry->ProgramDirectory) - 1;
          }

          for (Size i = 0; i < dirLength; ++i) {
            childEntry->ProgramDirectory[i] = name[i];
          }

          childEntry->ProgramDirectory[dirLength] = '\0';
        } else {
          CString::Copy(
            request.WorkingDirectory,
            childEntry->ProgramDirectory,
            sizeof(childEntry->ProgramDirectory)
          );
        }
      }
    }

    // send reply if caller requested one
    if (request.ReplyPortID != 0) {
      LoadELFResult result;

      result.Success = pid != InvalidProcessID;
      result.PID = pid;

      SendReply(
        request.ReplyPortID,
        &result,
        sizeof(result)
      );
    }
  }
}
