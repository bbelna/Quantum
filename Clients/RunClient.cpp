/**
 * @file Clients/RunClient.cpp
 * @brief Implements @ref @QClients::RunClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>
#include "RunClient.hpp"

namespace Quantum::Clients {
  ProcessID RunClient::LoadELF(
    const char* name,
    const char* workingDirectory,
    const void* elfData,
    Size elfSize,
    Size argumentCount,
    const char* const* arguments,
    UInt8 streamCount,
    const Quantum::Kernel::Memory::SharedBufferID* streamBufferIDs
  ) {
    Size argumentDataSize = 0;

    for (Size i = 0; i < argumentCount && arguments; ++i) {
      argumentDataSize += CString::Length(arguments[i]) + 1;
    }

    Size payloadSize = sizeof(LoadELFRequest) + elfSize + argumentDataSize;
    UIntPtr payloadAddress = AllocateBlock(payloadSize);

    if (payloadAddress == 0) return static_cast<ProcessID>(-1);

    auto* request = reinterpret_cast<LoadELFRequest*>(payloadAddress);

    request->ABIVersion = RunServerABIVersion;
    request->Operation = RunServerOperation::LoadELF;
    request->ELFSize = static_cast<UInt32>(elfSize);
    request->ArgumentCount = static_cast<UInt32>(argumentCount);
    request->StreamCount = streamCount;

    if (streamBufferIDs) {
      for (UInt8 i = 0; i < streamCount && i < 3; i++) {
        request->StreamBufferIDs[i] = streamBufferIDs[i];
      }
    }

    CString::Copy(name, request->Name, sizeof(request->Name));
    CString::Copy(
      workingDirectory,
      request->WorkingDirectory,
      sizeof(request->WorkingDirectory)
    );

    Byte::Copy(
      reinterpret_cast<void*>(payloadAddress + sizeof(LoadELFRequest)),
      elfData,
      elfSize
    );

    if (argumentDataSize > 0 && arguments) {
      UInt8* argumentDestination = reinterpret_cast<UInt8*>(
        payloadAddress + sizeof(LoadELFRequest) + elfSize
      );

      for (Size i = 0; i < argumentCount; ++i) {
        Size length = CString::Length(arguments[i]) + 1;

        Byte::Copy(argumentDestination, arguments[i], length);

        argumentDestination += length;
      }
    }

    LoadELFResult result = InvokeOSBuffer<
      LoadELFResult, LoadELFRequest
    >(RunServerPortID, request, payloadSize);

    FreeBlock(payloadAddress);

    return result.Success
      ? result.PID
      : static_cast<ProcessID>(-1);
  }

  void RunClient::SetWorkingDirectory(
    ProcessID pid,
    const char* path
  ) {
    SetWorkingDirectoryRequest request = {};

    request.ABIVersion = RunServerABIVersion;
    request.Operation = RunServerOperation::SetWorkingDirectory;
    request.PID = pid;

    CString::Copy(path, request.Path, sizeof(request.Path));

    SendOS(RunServerPortID, request);
  }

  bool RunClient::GetWorkingDirectory(
    ProcessID pid,
    char* outPath,
    Size outPathSize
  ) {
    GetWorkingDirectoryRequest request = {};

    request.ABIVersion = RunServerABIVersion;
    request.Operation = RunServerOperation::GetWorkingDirectory;
    request.PID = pid;

    GetWorkingDirectoryResult result = InvokeOS<
      GetWorkingDirectoryResult,
      GetWorkingDirectoryRequest
    >(RunServerPortID, 0, request);

    if (!result.Success || !outPath) return false;

    CString::Copy(result.Path, outPath, outPathSize);

    return true;
  }

  bool RunClient::GetProgramDirectory(
    ProcessID pid,
    char* outPath,
    Size outPathSize
  ) {
    GetProgramDirectoryRequest request = {};

    request.ABIVersion = RunServerABIVersion;
    request.Operation = RunServerOperation::GetProgramDirectory;
    request.PID = pid;

    GetProgramDirectoryResult result = InvokeOS<
      GetProgramDirectoryResult,
      GetProgramDirectoryRequest
    >(RunServerPortID, 0, request);

    if (!result.Success || !outPath) return false;

    CString::Copy(result.Path, outPath, outPathSize);

    return true;
  }
}
