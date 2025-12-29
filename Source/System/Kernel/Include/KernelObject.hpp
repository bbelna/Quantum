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
  struct BlockDeviceObject;
  struct InputDeviceObject;
  struct IRQLineObject;

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
        IPCPort = 1,
        BlockDevice = 2,
        InputDevice = 3,
        IRQLine = 4
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

      /**
       * Creates a kernel object for a block device.
       * @param deviceId
       *   Block device identifier.
       * @return
       *   Object pointer on success; nullptr on failure.
       */
      static BlockDeviceObject* CreateBlockDeviceObject(UInt32 deviceId);

      /**
       * Creates a kernel object for an input device.
       * @param deviceId
       *   Input device identifier.
       * @return
       *   Object pointer on success; nullptr on failure.
       */
      static InputDeviceObject* CreateInputDeviceObject(UInt32 deviceId);

      /**
       * Creates a kernel object for an IRQ line.
       * @param irq
       *   IRQ line number.
       * @return
       *   Object pointer on success; nullptr on failure.
       */
      static IRQLineObject* CreateIRQLineObject(UInt32 irq);

    protected:
      /**
       * Default destroy callback.
       * @param obj
       *   Kernel object pointer.
       */
      static void DefaultDestroy(KernelObject* obj);

      /**
       * Destroys an IPC port object.
       * @param obj
       *   Kernel object pointer.
       */
      static void DestroyIPCPortObject(KernelObject* obj);

      /**
       * Destroys a block device object.
       * @param obj
       *   Kernel object pointer.
       */
      static void DestroyBlockDeviceObject(KernelObject* obj);

      /**
       * Destroys an input device object.
       * @param obj
       *   Kernel object pointer.
       */
      static void DestroyInputDeviceObject(KernelObject* obj);

      /**
       * Destroys an IRQ line object.
       * @param obj
       *   Kernel object pointer.
       */
      static void DestroyIRQLineObject(KernelObject* obj);
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

  /**
   * Block device kernel object.
   */
  class BlockDeviceObject : public KernelObject {
    public:
      /**
       * Constructs a block device object.
       * @param device
       *   Block device identifier.
       */
      explicit BlockDeviceObject(UInt32 device);

      /**
       * Block device identifier.
       */
      UInt32 deviceId;
  };

  /**
   * Input device kernel object.
   */
  class InputDeviceObject : public KernelObject {
    public:
      /**
       * Constructs an input device object.
       * @param device
       *   Input device identifier.
       */
      explicit InputDeviceObject(UInt32 device);

      /**
       * Input device identifier.
       */
      UInt32 deviceId;
  };

  /**
   * IRQ line kernel object.
   */
  class IRQLineObject : public KernelObject {
    public:
      /**
       * Constructs an IRQ line object.
       * @param irq
       *   IRQ line number.
       */
      explicit IRQLineObject(UInt32 irq);

      /**
       * IRQ line number.
       */
      UInt32 irqLine;
  };
}
