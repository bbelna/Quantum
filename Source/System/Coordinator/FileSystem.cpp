/**
 * @file System/Coordinator/FileSystem.cpp
 * @brief Coordinator file system broker.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Task.hpp>

#include "FileSystem.hpp"

namespace Quantum::System::Coordinator {
  using ABI::Console;
  using ABI::IPC;
  using ABI::Task;

  void FileSystem::Initialize() {
    if (_portId != 0) {
      return;
    }

    _portId = IPC::CreatePort();

    if (_portId == 0) {
      Console::WriteLine("Coordinator: failed to create file system port");
      return;
    }

    if (_portId != IPC::Ports::FileSystem) {
      Console::WriteLine("Coordinator: file system port id mismatch");
    }

    _portHandle = IPC::OpenPort(
      _portId,
      IPC::RightReceive | IPC::RightManage
    );

    if (_portHandle == 0) {
      Console::WriteLine("Coordinator: failed to open file system port handle");
    }
  }

  FileSystem::Service* FileSystem::FindFirstService() {
    for (UInt32 i = 0; i < _maxServices; ++i) {
      if (_services[i].portId != 0) {
        return &_services[i];
      }
    }

    return nullptr;
  }

  FileSystem::Service* FileSystem::FindService(
    ABI::FileSystem::Type type
  ) {
    for (UInt32 i = 0; i < _maxServices; ++i) {
      if (_services[i].portId != 0 && _services[i].type == type) {
        return &_services[i];
      }
    }

    return nullptr;
  }

  void FileSystem::RegisterService(
    ABI::FileSystem::Type type,
    UInt32 portId
  ) {
    if (portId == 0) {
      return;
    }

    Service* existing = FindService(type);

    if (existing) {
      existing->portId = portId;

      return;
    }

    for (UInt32 i = 0; i < _maxServices; ++i) {
      if (_services[i].portId == 0) {
        _services[i].type = type;
        _services[i].portId = portId;

        return;
      }
    }
  }

  ABI::FileSystem::Handle FileSystem::AllocateHandle(
    UInt32 servicePort,
    ABI::FileSystem::Handle serviceHandle,
    bool isVolume
  ) {
    for (UInt32 i = 0; i < _maxHandles; ++i) {
      if (!_handles[i].inUse) {
        ABI::FileSystem::Handle handle = _nextHandle++;

        if (handle == 0) {
          handle = _nextHandle++;
        }

        _handles[i].inUse = true;
        _handles[i].isVolume = isVolume;
        _handles[i].userHandle = handle;
        _handles[i].serviceHandle = serviceHandle;
        _handles[i].servicePort = servicePort;

        return handle;
      }
    }

    return 0;
  }

  FileSystem::HandleMap* FileSystem::FindHandle(
    ABI::FileSystem::Handle userHandle,
    bool expectVolume
  ) {
    for (UInt32 i = 0; i < _maxHandles; ++i) {
      if (
        _handles[i].inUse &&
        _handles[i].userHandle == userHandle &&
        _handles[i].isVolume == expectVolume
      ) {
        return &_handles[i];
      }
    }

    return nullptr;
  }

  void FileSystem::ReleaseHandle(ABI::FileSystem::Handle userHandle) {
    for (UInt32 i = 0; i < _maxHandles; ++i) {
      if (_handles[i].inUse && _handles[i].userHandle == userHandle) {
        _handles[i].inUse = false;
        _handles[i].isVolume = false;
        _handles[i].userHandle = 0;
        _handles[i].serviceHandle = 0;
        _handles[i].servicePort = 0;

        return;
      }
    }
  }

  void FileSystem::StorePendingReply(UInt32 senderId, UInt32 handle) {
    if (senderId == 0 || handle == 0) {
      return;
    }

    for (UInt32 i = 0; i < _maxPendingReplies; ++i) {
      if (_pendingReplies[i].inUse && _pendingReplies[i].senderId == senderId) {
        if (_pendingReplies[i].handle != 0) {
          IPC::CloseHandle(_pendingReplies[i].handle);
        }

        _pendingReplies[i].handle = handle;

        return;
      }
    }

    for (UInt32 i = 0; i < _maxPendingReplies; ++i) {
      if (!_pendingReplies[i].inUse) {
        _pendingReplies[i].inUse = true;
        _pendingReplies[i].senderId = senderId;
        _pendingReplies[i].handle = handle;

        return;
      }
    }

    IPC::CloseHandle(handle);
  }

  UInt32 FileSystem::TakePendingReply(UInt32 senderId) {
    if (senderId == 0) {
      return 0;
    }

    for (UInt32 i = 0; i < _maxPendingReplies; ++i) {
      if (_pendingReplies[i].inUse && _pendingReplies[i].senderId == senderId) {
        UInt32 handle = _pendingReplies[i].handle;

        _pendingReplies[i].inUse = false;
        _pendingReplies[i].senderId = 0;
        _pendingReplies[i].handle = 0;

        return handle;
      }
    }

    return 0;
  }

  void FileSystem::ProcessPending() {
    if (_portId == 0) {
      return;
    }

    UInt32 receiveId = _portHandle != 0 ? _portHandle : _portId;

    for (;;) {
      IPC::Message msg {};

      if (IPC::TryReceive(receiveId, msg) != 0) {
        break;
      }

      IPC::Handle transferHandle = 0;

      if (IPC::TryGetHandleMessage(msg, transferHandle)) {
        StorePendingReply(msg.senderId, transferHandle);

        continue;
      }

      if (msg.length < ABI::FileSystem::messageHeaderBytes) {
        continue;
      }

      ABI::FileSystem::ServiceMessage request {};
      UInt32 copyBytes = msg.length;

      if (copyBytes > sizeof(request)) {
        copyBytes = sizeof(request);
      }

      for (UInt32 i = 0; i < copyBytes; ++i) {
        reinterpret_cast<UInt8*>(&request)[i] = msg.payload[i];
      }

      if (
        request.op == static_cast<UInt32>(
          ABI::FileSystem::Operation::RegisterService
        )
      ) {
        RegisterService(
          static_cast<ABI::FileSystem::Type>(request.arg0),
          request.arg1
        );

        IPC::Handle replyHandle = 0;

        if (request.replyPortId != 0) {
          replyHandle = IPC::OpenPort(request.replyPortId, IPC::RightSend);
        } else {
          replyHandle = TakePendingReply(msg.senderId);
        }

        if (replyHandle != 0) {
          ABI::FileSystem::ServiceMessage response {};
          IPC::Message reply {};

          response.op = request.op;
          response.status = 0;
          response.replyPortId = 0;
          response.arg0 = 0;
          response.arg1 = 0;
          response.arg2 = 0;
          response.dataLength = 0;

          reply.length = ABI::FileSystem::messageHeaderBytes;

          for (UInt32 i = 0; i < reply.length; ++i) {
            reply.payload[i] = reinterpret_cast<UInt8*>(&response)[i];
          }

          IPC::Send(replyHandle, reply);
          IPC::CloseHandle(replyHandle);
        }

        continue;
      }

      ABI::FileSystem::Operation op
        = static_cast<ABI::FileSystem::Operation>(request.op);
      IPC::Handle clientReplyHandle = 0;

      if (request.replyPortId != 0) {
        clientReplyHandle = IPC::OpenPort(request.replyPortId, IPC::RightSend);
      } else {
        clientReplyHandle = TakePendingReply(msg.senderId);
      }

      if (clientReplyHandle == 0) {
        continue;
      }

      ABI::FileSystem::ServiceMessage response {};

      response.op = request.op;
      response.status = 1;
      response.replyPortId = 0;
      response.arg0 = 0;
      response.arg1 = 0;
      response.arg2 = 0;
      response.dataLength = 0;

      auto sendReply = [&]() {
        IPC::Message reply {};

        reply.length
          = ABI::FileSystem::messageHeaderBytes + response.dataLength;

        if (reply.length > IPC::maxPayloadBytes) {
          reply.length = ABI::FileSystem::messageHeaderBytes;
          response.dataLength = 0;
        }

        for (UInt32 i = 0; i < reply.length; ++i) {
          reply.payload[i] = reinterpret_cast<UInt8*>(&response)[i];
        }

        if (clientReplyHandle != 0) {
          IPC::Send(clientReplyHandle, reply);
          IPC::CloseHandle(clientReplyHandle);
          clientReplyHandle = 0;
        }
      };

      if (op == ABI::FileSystem::Operation::ListVolumes) {
        UInt32 entryBytes
          = static_cast<UInt32>(sizeof(ABI::FileSystem::VolumeEntry));
        UInt32 maxEntries = request.arg1;
        UInt32 maxPayloadEntries = entryBytes == 0
          ? 0
          : (ABI::FileSystem::messageDataBytes / entryBytes);
        UInt32 remaining = maxEntries;

        if (maxPayloadEntries < remaining) {
          remaining = maxPayloadEntries;
        }

        UInt32 count = 0;

        for (UInt32 i = 0; i < _maxServices && remaining > 0; ++i) {
          if (_services[i].portId == 0) {
            continue;
          }

          UInt32 replyPortId = IPC::CreatePort();

          if (replyPortId == 0) {
            continue;
          }

          IPC::Handle replyHandle = IPC::OpenPort(
            replyPortId,
            IPC::RightReceive | IPC::RightManage | IPC::RightSend
          );

          if (replyHandle == 0) {
            IPC::DestroyPort(replyPortId);

            continue;
          }

          ABI::FileSystem::ServiceMessage serviceRequest = request;

          serviceRequest.replyPortId = 0;
          serviceRequest.arg1 = remaining;

          IPC::Message forward {};

          forward.length
            = ABI::FileSystem::messageHeaderBytes + serviceRequest.dataLength;

          for (UInt32 j = 0; j < forward.length; ++j) {
            forward.payload[j]
              = reinterpret_cast<UInt8*>(&serviceRequest)[j];
          }

          IPC::Handle serviceHandle = IPC::OpenPort(
            _services[i].portId,
            IPC::RightSend
          );

          if (serviceHandle == 0) {
            IPC::DestroyPort(replyHandle);
            IPC::CloseHandle(replyHandle);

            continue;
          }

          if (IPC::SendHandle(
            serviceHandle,
            replyHandle,
            IPC::RightSend
          ) != 0) {
            IPC::CloseHandle(serviceHandle);
            IPC::DestroyPort(replyHandle);
            IPC::CloseHandle(replyHandle);

            continue;
          }

          if (IPC::Send(serviceHandle, forward) != 0) {
            IPC::CloseHandle(serviceHandle);
            IPC::DestroyPort(replyHandle);
            IPC::CloseHandle(replyHandle);

            continue;
          }

          IPC::CloseHandle(serviceHandle);

          IPC::Message serviceReply {};

          if (IPC::Receive(replyHandle, serviceReply) != 0) {
            IPC::DestroyPort(replyHandle);
            IPC::CloseHandle(replyHandle);

            continue;
          }

          ABI::FileSystem::ServiceMessage serviceResponse {};
          UInt32 serviceBytes = serviceReply.length;

          if (serviceBytes > sizeof(serviceResponse)) {
            serviceBytes = sizeof(serviceResponse);
          }

          for (UInt32 j = 0; j < serviceBytes; ++j) {
            reinterpret_cast<UInt8*>(&serviceResponse)[j]
              = serviceReply.payload[j];
          }

          UInt32 availableBytes = serviceResponse.dataLength;
          UInt32 entries = entryBytes == 0 ? 0 : (availableBytes / entryBytes);

          for (UInt32 j = 0; j < entries && remaining > 0; ++j) {
            UInt32 destOffset = count * entryBytes;
            UInt32 srcOffset = j * entryBytes;

            for (UInt32 k = 0; k < entryBytes; ++k) {
              response.data[destOffset + k]
                = serviceResponse.data[srcOffset + k];
            }

            ++count;
            --remaining;
          }

          IPC::DestroyPort(replyHandle);
          IPC::CloseHandle(replyHandle);
        }

        response.status = count;
        response.dataLength = count * entryBytes;

        sendReply();

        continue;
      }

      if (op == ABI::FileSystem::Operation::OpenVolume) {
        response.status = 0;

        for (UInt32 i = 0; i < _maxServices; ++i) {
          if (_services[i].portId == 0) {
            continue;
          }

          UInt32 replyPortId = IPC::CreatePort();

          if (replyPortId == 0) {
            continue;
          }

          IPC::Handle replyHandle = IPC::OpenPort(
            replyPortId,
            IPC::RightReceive | IPC::RightManage | IPC::RightSend
          );

          if (replyHandle == 0) {
            IPC::DestroyPort(replyPortId);

            continue;
          }

          ABI::FileSystem::ServiceMessage serviceRequest = request;

          serviceRequest.replyPortId = 0;

          IPC::Message forward {};

          forward.length
            = ABI::FileSystem::messageHeaderBytes + serviceRequest.dataLength;

          for (UInt32 j = 0; j < forward.length; ++j) {
            forward.payload[j]
              = reinterpret_cast<UInt8*>(&serviceRequest)[j];
          }

          IPC::Handle serviceHandle = IPC::OpenPort(
            _services[i].portId,
            IPC::RightSend
          );

          if (serviceHandle == 0) {
            IPC::DestroyPort(replyHandle);
            IPC::CloseHandle(replyHandle);

            continue;
          }

          if (IPC::SendHandle(
            serviceHandle,
            replyHandle,
            IPC::RightSend
          ) != 0) {
            IPC::CloseHandle(serviceHandle);
            IPC::DestroyPort(replyHandle);
            IPC::CloseHandle(replyHandle);

            continue;
          }

          if (IPC::Send(serviceHandle, forward) != 0) {
            IPC::CloseHandle(serviceHandle);
            IPC::DestroyPort(replyHandle);
            IPC::CloseHandle(replyHandle);

            continue;
          }

          IPC::CloseHandle(serviceHandle);

          IPC::Message serviceReply {};

          if (IPC::Receive(replyHandle, serviceReply) != 0) {
            IPC::DestroyPort(replyHandle);
            IPC::CloseHandle(replyHandle);

            continue;
          }

          ABI::FileSystem::ServiceMessage serviceResponse {};
          UInt32 serviceBytes = serviceReply.length;

          if (serviceBytes > sizeof(serviceResponse)) {
            serviceBytes = sizeof(serviceResponse);
          }

          for (UInt32 j = 0; j < serviceBytes; ++j) {
            reinterpret_cast<UInt8*>(&serviceResponse)[j]
              = serviceReply.payload[j];
          }

          if (serviceResponse.status != 0) {
            ABI::FileSystem::Handle handle = AllocateHandle(
              _services[i].portId,
              static_cast<ABI::FileSystem::Handle>(serviceResponse.status),
              true
            );

            response.status = handle;
            response.dataLength = 0;

            IPC::DestroyPort(replyHandle);
            IPC::CloseHandle(replyHandle);

            sendReply();

            break;
          }

          IPC::DestroyPort(replyHandle);
          IPC::CloseHandle(replyHandle);
        }

        if (response.status == 0) {
          sendReply();
        }

        continue;
      }

      bool expectVolumeHandle = false;
      bool expectFileHandle = false;

      switch (op) {
        case ABI::FileSystem::Operation::GetVolumeInfo:
        case ABI::FileSystem::Operation::SetVolumeLabel:
        case ABI::FileSystem::Operation::CloseVolume:
        case ABI::FileSystem::Operation::Open:
        case ABI::FileSystem::Operation::CreateDirectory:
        case ABI::FileSystem::Operation::CreateFile:
        case ABI::FileSystem::Operation::Remove:
        case ABI::FileSystem::Operation::Rename:
          expectVolumeHandle = true;
          break;
        case ABI::FileSystem::Operation::Close:
        case ABI::FileSystem::Operation::ReadDirectory:
        case ABI::FileSystem::Operation::Read:
        case ABI::FileSystem::Operation::Write:
        case ABI::FileSystem::Operation::Stat:
        case ABI::FileSystem::Operation::Seek:
          expectFileHandle = true;
          break;
        default:
          break;
      }

      HandleMap* mapped = nullptr;

      if (expectVolumeHandle) {
        mapped = FindHandle(request.arg0, true);
      } else if (expectFileHandle) {
        mapped = FindHandle(request.arg0, false);
      }

      UInt32 servicePort = 0;
      ABI::FileSystem::Handle serviceHandle = 0;

      if (mapped) {
        servicePort = mapped->servicePort;
        serviceHandle = mapped->serviceHandle;
      } else if (!expectVolumeHandle && !expectFileHandle) {
        Service* first = FindFirstService();

        if (first) {
          servicePort = first->portId;
        }
      }

      if (servicePort == 0) {
        sendReply();

        continue;
      }

      UInt32 replyPortId = IPC::CreatePort();

      if (replyPortId == 0) {
        sendReply();

        continue;
      }

      ABI::FileSystem::ServiceMessage serviceRequest = request;

      serviceRequest.replyPortId = 0;

      if (mapped) {
        serviceRequest.arg0 = serviceHandle;
      }

      IPC::Message forward {};

      forward.length
        = ABI::FileSystem::messageHeaderBytes + serviceRequest.dataLength;

      for (UInt32 i = 0; i < forward.length; ++i) {
        forward.payload[i] = reinterpret_cast<UInt8*>(&serviceRequest)[i];
      }

      IPC::Handle replyHandle = IPC::OpenPort(
        replyPortId,
        IPC::RightReceive | IPC::RightManage | IPC::RightSend
      );

      if (replyHandle == 0) {
        IPC::DestroyPort(replyPortId);
        sendReply();

        continue;
      }

      IPC::Handle serviceSendHandle = IPC::OpenPort(
        servicePort,
        IPC::RightSend
      );

      if (serviceSendHandle == 0) {
        IPC::DestroyPort(replyHandle);
        IPC::CloseHandle(replyHandle);
        sendReply();

        continue;
      }

      if (IPC::SendHandle(
        serviceSendHandle,
        replyHandle,
        IPC::RightSend
      ) != 0) {
        IPC::CloseHandle(serviceSendHandle);
        IPC::DestroyPort(replyHandle);
        IPC::CloseHandle(replyHandle);

        sendReply();

        continue;
      }

      if (IPC::Send(serviceSendHandle, forward) != 0) {
        IPC::CloseHandle(serviceSendHandle);
        IPC::DestroyPort(replyHandle);
        IPC::CloseHandle(replyHandle);

        sendReply();

        continue;
      }

      IPC::CloseHandle(serviceSendHandle);

      IPC::Message serviceReply {};

      if (IPC::Receive(replyHandle, serviceReply) != 0) {
        IPC::DestroyPort(replyHandle);
        IPC::CloseHandle(replyHandle);

        sendReply();

        continue;
      }

      UInt32 serviceBytes = serviceReply.length;

      if (serviceBytes > sizeof(response)) {
        serviceBytes = sizeof(response);
      }

      for (UInt32 i = 0; i < serviceBytes; ++i) {
        reinterpret_cast<UInt8*>(&response)[i] = serviceReply.payload[i];
      }

      IPC::DestroyPort(replyHandle);
      IPC::CloseHandle(replyHandle);

      if (op == ABI::FileSystem::Operation::Open) {
        if (response.status != 0) {
          ABI::FileSystem::Handle handle = AllocateHandle(
            servicePort,
            static_cast<ABI::FileSystem::Handle>(response.status),
            false
          );

          response.status = handle;
        }
      } else if (op == ABI::FileSystem::Operation::Close) {
        if (response.status == 0) {
          ReleaseHandle(request.arg0);
        }
      } else if (op == ABI::FileSystem::Operation::CloseVolume) {
        if (response.status == 0) {
          ReleaseHandle(request.arg0);
        }
      }

      sendReply();
    }

    Task::Yield();
  }
}
