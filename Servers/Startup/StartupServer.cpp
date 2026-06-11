/**
 * @file Servers/Startup/StartupServer.cpp
 * @brief Implements @ref @QStpSrv::StartupServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "StartupServer.hpp"

namespace Quantum::Servers::Startup {
  StartupServer::StartupServer(
    ServerLog& log,
    StartupBundle* bundle
  ) :
    Server(StartupPortID),
    _kernel(),
    _log(log),
    _bundle(bundle)
  {
  }

  Int32 StartupServer::Start() {
    if (!_portHandle) {
      _log.Error("Failed to open IPC port");

      return -1;
    }

    _spawnRawSystemEntries();

    _spawnELF(StartupBundleEntryType::System);
    _spawnELF(StartupBundleEntryType::Driver);
    _spawnELF(StartupBundleEntryType::FileSystem);

    _spawnFromDisk(StartupBundleEntryType::Driver);
    _spawnFromDisk(StartupBundleEntryType::FileSystem);
    _spawnFromDisk(StartupBundleEntryType::System);
    _spawnFromDisk(StartupBundleEntryType::Application);

    return 0;
  }

  void StartupServer::_spawnRawSystemEntries() {
    Size spawnCount = 0;

    // first pass: spawn all Raw format servers (including Run server)
    for (
      Size entryIndex = 0;
      entryIndex < _bundle->Header->EntryCount;
      entryIndex++
    ) {
      const StartupBundleEntry& entry = _bundle->Entries[entryIndex];

      if (
        entry.Type == StartupBundleEntryType::System &&
        entry.Format == StartupBundleEntryFormatType::Raw
      ) {
        _log.Info("Spawning \"%s\"", entry.Name);

        ProcessID processID = _bundle->SpawnEntry(entry);

        if (processID == InvalidProcessID) {
          _log.Error("Failed to spawn \"%s\"", entry.Name);
        } else {
          spawnCount++;
        }
      }
    }

    for (Size readyCount = 0; readyCount < spawnCount;) {
      IPCMessage* message = _kernel.ReceiveIPCMessage(_portHandle);

      if (message) {
        StartupRequest* request = reinterpret_cast<StartupRequest*>(
          message->Payload
        );

        if (request->ABIVersion == StartupABIVersion) {
          switch (request->Operation) {
            case StartupOperation::Ready: {
              ++readyCount;

              _log.Trace(
                "Received ready from PID %u (%u/%u)",
                message->SendingProcessID,
                readyCount,
                spawnCount
              );

              break;
            } default: {
              _log.Warning(
                "Received unknown operation %u from PID %u",
                static_cast<UInt32>(request->Operation),
                message->SendingProcessID
              );

              break;
            }
          }
        } else {
          _log.Warning(
            "Received message with unknown ABI version %u from PID %u",
            request->ABIVersion,
            message->SendingProcessID
          );
        }

        delete message;
      }
    }
  }

  void StartupServer::_spawnELF(StartupBundleEntryType type) {
    // check if there are any ELF system entries to send
    bool hasELFEntryOfType = false;

    for (
      Size entryIndex = 0;
      entryIndex < _bundle->Header->EntryCount;
      entryIndex++
    ) {
      if (
        _bundle->Entries[entryIndex].Format
          == StartupBundleEntryFormatType::ELF &&
        _bundle->Entries[entryIndex].Type == type
      ) {
        hasELFEntryOfType = true;

        break;
      }
    }

    if (!hasELFEntryOfType) {
      return;
    }

    // wait for the Run server's IPC port to exist, then open it
    _kernel.WaitForIPCPort(RunServerPortID);

    IPCPortResourceID handle = _kernel.OpenIPCPort(
      RunServerPortID,
      IPCPortRights::Send
    );

    if (handle == InvalidIPCPortResourceID) {
      _log.Error("Failed to connect to RunServer");

      return;
    }

    UInt8* bundleBase = reinterpret_cast<UInt8*>(
      const_cast<StartupBundleHeader*>(_bundle->Header)
    );

    for (
      Size entryIndex = 0;
      entryIndex < _bundle->Header->EntryCount;
      entryIndex++
    ) {
      const StartupBundleEntry& entry = _bundle->Entries[entryIndex];

      if (
        entry.Format != StartupBundleEntryFormatType::ELF ||
        entry.Type != type || (
          type == StartupBundleEntryType::Application &&
          !(entry.Flags & StartupBundleEntryFlags::Required)
        )
      ) {
        continue;
      }

      _log.Info("Spawning \"%s\"", entry.Name);

      // build the IPC payload: LoadELFRequest header + raw ELF data
      Size payloadSize = sizeof(LoadELFRequest) + entry.Size;
      UIntPtr payloadAddress = AllocateBlock(payloadSize);

      if (payloadAddress == 0) {
        _log.Error(
          "Failed to allocate IPC payload for \"%s\"",
          entry.Name
        );

        continue;
      }

      LoadELFRequest* payload = reinterpret_cast<LoadELFRequest*>(
        payloadAddress
      );

      payload->ABIVersion = RunServerABIVersion;
      payload->Operation = RunServerOperation::LoadELF;
      payload->ReplyPortID = 0;

      // copy entry name
      for (
        Size nameIndex = 0;
        nameIndex < 64 && nameIndex < 32;
        ++nameIndex
      ) {
        payload->Name[nameIndex] = nameIndex < 32
          ? entry.Name[nameIndex]
          : '\0';
      }

      // copy ELF data after the request struct
      Byte::Copy(
        reinterpret_cast<void*>(payloadAddress + sizeof(LoadELFRequest)),
        bundleBase + entry.Offset,
        entry.Size
      );

      _kernel.SendIPCMessage(
        handle,
        payload,
        payloadSize
      );
      _kernel.FreeBlock(payloadAddress);

      // wait for each Required entry to signal Ready before spawning the
      // next one so that dependent servers (e.g. GFX depending on Devices)
      // find their dependencies' IPC ports already open
      if (entry.Flags & StartupBundleEntryFlags::Required) {
        for (;;) {
          IPCMessage* readyMessage = _kernel.ReceiveIPCMessage(_portHandle);

          if (!readyMessage) continue;

          StartupRequest* readyRequest = reinterpret_cast<StartupRequest*>(
            readyMessage->Payload
          );

          bool isReady
            = readyMessage->PayloadSizeInBytes >= sizeof(StartupRequest)
           && readyRequest->ABIVersion == StartupABIVersion
           && readyRequest->Operation == StartupOperation::Ready;

          if (isReady) {
            _log.Trace(
              "Ready from PID %u",
              readyMessage->SendingProcessID
            );
          }

          delete readyMessage;

          if (isReady) {
            break;
          }
        }
      }
    }

    _kernel.CloseIPCPort(handle);
  }

  void StartupServer::_spawnFromDisk(StartupBundleEntryType type) {
    UInt8* bundleBase = reinterpret_cast<UInt8*>(
      const_cast<StartupBundleHeader*>(_bundle->Header)
    );

    for (
      Size entryIndex = 0;
      entryIndex < _bundle->Header->EntryCount;
      entryIndex++
    ) {
      const StartupBundleEntry& entry = _bundle->Entries[entryIndex];

      if (
        entry.Format != StartupBundleEntryFormatType::DiskELF ||
        entry.Type != type
      ) {
        continue;
      }

      // extract the disk path from the bundle payload area
      const char* diskPath = reinterpret_cast<const char*>(
        bundleBase + entry.Offset
      );

      char path[FileSystemMaxPathLength];

      // build the full file system path
      // TODO: actual volume label
      CString::Format(
        path,
        sizeof(path),
        "QUANTUM/%s",
        diskPath
      );

      FileSystemFileStat stat = {};
      FileSystemClient fileSystem;

      _log.Info("Spawning \"%s\"", path);

      if (
        !fileSystem.Stat(path, &stat) ||
        stat.Type != FileSystemEntryType::Regular
      ) {
        _log.Error(
          "\"%s\" not found on QUANTUM",
          entry.Name
        );

        continue;
      }

      if (stat.Size == 0) {
        _log.Error(
          "\"%s\" is empty",
          entry.Name
        );

        continue;
      }

      FileHandle fileHandle = fileSystem.Open(
        path,
        Enum::ToBase(FileSystemOpenFlags::Read)
      );

      if (fileHandle == 0) {
        _log.Error(
          "Failed to open \"%s\"",
          entry.Name
        );

        continue;
      }

      UIntPtr buffer = _kernel.AllocateBlock(stat.Size);

      if (buffer == 0) {
        _log.Error(
          "Out of memory for \"%s\"",
          entry.Name
        );

        fileSystem.Close(fileHandle);

        continue;
      }

      Int32 bytesRead = fileSystem.Read(
        fileHandle,
        reinterpret_cast<void*>(buffer),
        stat.Size,
        0
      );

      fileSystem.Close(fileHandle);

      if (bytesRead <= 0) {
        _log.Error(
          "Failed to read \"%s\"",
          entry.Name
        );

        _kernel.FreeBlock(buffer);

        continue;
      }

      RunClient runClient;
      ProcessID processID = runClient.LoadELF(
        path,
        path,
        reinterpret_cast<const void*>(buffer),
        static_cast<Size>(bytesRead)
      );

      _kernel.FreeBlock(buffer);

      if (processID == static_cast<ProcessID>(-1)) {
        _log.Error(
          "Failed to load ELF for \"%s\"",
          entry.Name
        );

        continue;
      }

      _log.Trace(
        "\"%s\" spawned as PID %u",
        entry.Name,
        static_cast<UInt32>(processID)
      );

      // wait for each Required entry to signal Ready before spawning the
      // next one so that dependent servers (e.g. AppServer depending on
      // ContextServer) find their dependencies' IPC ports already open
      if (entry.Flags & StartupBundleEntryFlags::Required) {
        for (;;) {
          IPCMessage* readyMessage = _kernel.ReceiveIPCMessage(_portHandle);

          if (!readyMessage) {
            continue;
          }

          StartupRequest* readyRequest
            = reinterpret_cast<StartupRequest*>(
              readyMessage->Payload
            );

          bool isReady =
            readyMessage->PayloadSizeInBytes >= sizeof(StartupRequest) &&
            readyRequest->ABIVersion == StartupABIVersion &&
            readyRequest->Operation == StartupOperation::Ready;

          if (isReady) {
            _log.Trace(
              "Ready from PID %u",
              readyMessage->SendingProcessID
            );
          }

          delete readyMessage;

          if (isReady) {
            break;
          }
        }
      }
    }
  }
}

/**
 * @brief Entry point for @ref @QStpSrv::StartupServer.
 * @param argumentCount The number of arguments.
 * @param arguments The arguments.
 * @param environment The environment variables.
 * @return `0` on success, non-zero on failure.
 *
 * Initializes @ref @QStpSrv::StartupServer and performs the boot
 * sequence. The server exits after all startup entries have been
 * spawned and signaled ready.
 */
int Main(int argumentCount, char** arguments, char** environment) {
  UIntPtr address = *reinterpret_cast<const UIntPtr*>(arguments[0]);
  StartupBundle* bundle = StartupBundle::FromVirtualAddress(address);

  KernelClient kernel;
  ServerLog log(kernel);

  StartupServer server(log, bundle);

  return server.Start();
}
