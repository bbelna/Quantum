/**
 * @file System/Kernel/Include/KernelObject.hpp
 * @brief Kernel object base definitions.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  struct IPCPortObject;

  /**
   * Kernel object.
   */
  class KernelObject {
    public:
      /**
       * Kernel object type identifiers.
       */
      enum class Type : UInt32 {
        None = 0,
        IPCPort = 1
      };

      /**
       * Object type.
       */
      Type type;

      /**
       * Reference count.
       */
      UInt32 refCount;

      /**
       * Optional destroy callback.
       */
      void (*destroy)(KernelObject*);

      /**
       * Constructs a kernel object.
       * @param type
       *   Kernel object type.
       * @param destroy
       *   Optional destroy callback.
       */
      KernelObject(
        Type type = Type::None,
        void (*destroy)(KernelObject*) = nullptr
      );

      /**
       * Increments the kernel object refcount.
       */
      void AddRef();

      /**
       * Releases a kernel object reference.
       */
      void Release();

      /**
       * Creates a kernel object for an IPC port.
       * @param portId
       *   IPC port identifier.
       * @return
       *   Object pointer on success; nullptr on failure.
       */
      static IPCPortObject* CreateIPCPortObject(UInt32 portId);
  };

  /**
   * IPC port kernel object.
   */
  class IPCPortObject : public KernelObject {
    public:
      /**
       * Constructs an IPC port object.
       * @param port
       *   IPC port identifier.
       */
      explicit IPCPortObject(UInt32 port);

      /**
       * IPC port identifier.
       */
      UInt32 portId;
  };
}
