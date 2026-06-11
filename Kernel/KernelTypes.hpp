/**
 * @file Kernel/KernelTypes.hpp
 * @brief Declares core @ref @QKrnl types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Address.hpp>
#include <Quantum/Core/Math.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Structures.hpp>

#include "Concurrency/ConcurrencyTypes.hpp"
#include "Core/CoreTypes.hpp"
#include "Interrupts/InterruptTypes.hpp"

namespace Quantum::Kernel {
  struct KernelContext;

  /**
   * @brief Global pointer to the active @ref @QKrnl::KernelContext.
   *
   * Set by @ref @QKrnl::Kernel::Initialize during boot. Provides
   * lightweight access to kernel subsystems without depending on the
   * @ref @QKrnl::Kernel class.
   */
  inline KernelContext* Context = nullptr;


  namespace Handlers {
    class IMaydayHandler;
    class IOutOfMemoryHandler;

    class OutOfMemoryHandler;
  }

  namespace Bootstrap {
    struct InitialProcessInfo;
  }

  namespace Concurrency {
    struct Process;
    struct Thread;

    template <typename ValueType, typename Impl>
    class IAtomic;

    class IThreadContextManager;

    class FutexManager;
    class ProcessManager;
    class HybridCFSBitmapScheduler;

    template <typename LockStateType>
    class Spinlock;

    class ThreadManager;
  }

  namespace Drivers {
    namespace CPU {
      class ICPUDriver;
    }

    namespace Interrupts {
      template <typename InterruptVectorType>
      class IInterruptControllerDriver;
    }

    namespace Timers {
      class ITimerDriver;
    }
  }

  namespace Interrupts {
    struct IInterruptContext;

    template <typename FlagsType, typename Impl>
    class IInterruptControl;

    template <typename InterruptVectorType>
    class IInterruptManager;
  }

  namespace IPC {
    struct IPCMessage;
    struct IPCPort;

    class IPCPortService;
    class IPCPortRepository;
  }

  namespace Logging {
    class SpinlockLog;
  }

  namespace Memory {
    struct IAddressSpace;

    template<typename PhysAddr>
    struct BuddyFreeNode;

    struct MemoryMapping;
    struct MemoryMappingFlags;
    struct MemoryRegion;
    struct SharedBuffer;
    struct Stack;

    class IAddressSpaceAllocator;
    class IAddressTranslator;
    class IMemoryAllocator;
    class IMemoryMapper;

    template <typename T, UIntPtr MinAddress>
    class AddressSpaceMap;

    template<typename Address>
    class BuddyAllocator;

    class HeapAllocator;
    class MemoryPressureMonitor;

    template <typename T, Size N>
    class ObjectPool;

    class SharedBufferRepository;
    class StackManager;

    using ProcessAddressSpaceMap = AddressSpaceMap<MemoryMapping, 4096>;
    using KernelAddressSpaceMap = AddressSpaceMap<MemoryRegion, 0>;
  }

  namespace Resources {
    class KernelResourceManager;
  }

  namespace UserMode {
    struct UserModeEntryInfo;

    class IUserModeManager;
  }
}

using namespace Quantum::Core;
using namespace Quantum::Core::Math;

using namespace Quantum::Kernel;
using namespace Quantum::Kernel::Bootstrap;
using namespace Quantum::Kernel::Concurrency;
using namespace Quantum::Kernel::Drivers::CPU;
using namespace Quantum::Kernel::Drivers::Interrupts;
using namespace Quantum::Kernel::Drivers::Timers;
using namespace Quantum::Kernel::Handlers;
using namespace Quantum::Kernel::Interrupts;
using namespace Quantum::Kernel::IPC;
using namespace Quantum::Kernel::Logging;
using namespace Quantum::Kernel::Memory;
using namespace Quantum::Kernel::Resources;
using namespace Quantum::Kernel::UserMode;

using namespace Quantum::Structures::Lists;
using namespace Quantum::Structures::Nodes;
using namespace Quantum::Structures::Queues;
using namespace Quantum::Structures::Trees;
