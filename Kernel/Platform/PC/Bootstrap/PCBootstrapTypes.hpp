/**
 * @file Kernel/Platform/PC/Bootstrap/PCBootstrapTypes.hpp
 * @brief Declares @ref @QKrnlPC::Bootstrap types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

#if defined(ARCH_IA32)
#include <Arch/IA32/Bootstrap/IA32BootstrapTypes.hpp>
#include <Arch/IA32/Concurrency/IA32ConcurrencyTypes.hpp>
#include <Arch/IA32/Concurrency/IA32ThreadContextManager.hpp>
#include <Arch/IA32/Drivers/CPU/IA32CPUDriver.hpp>
#include <Arch/IA32/Handlers/IA32MaydayHandler.hpp>
#include <Arch/IA32/Interrupts/IA32InterruptConstants.hpp>
#include <Arch/IA32/Interrupts/IA32InterruptManager.hpp>
#include <Arch/IA32/IA32Constants.hpp>
#include <Arch/IA32/IA32LinkerSymbols.hpp>
#include <Arch/IA32/Memory/IA32AddressTranslator.hpp>
#include <Arch/IA32/Memory/IA32MemoryAllocator.hpp>
#include <Arch/IA32/Memory/IA32MemoryMapper.hpp>
#include <Arch/IA32/Memory/IA32StackManagerConfigurationProvider.hpp>
#include <Arch/IA32/UserMode/IA32UserModeManager.hpp>
#endif

#include <Bootstrap/IKernelInitializer.hpp>
#include <KernelConstants.hpp>
#include <Kernel.hpp>
#include <KernelContext.hpp>
#include <Logging/ScreenLogSink.hpp>
#include <Platform/PC/Drivers/Bus/PCBusDriverTypes.hpp>
#include <Platform/PC/Drivers/DMA/PCDMADriverTypes.hpp>
#include <Platform/PC/Drivers/Graphics/PCGraphicsDriverTypes.hpp>
#include <Platform/PC/Drivers/Interrupts/PCInterruptDriverTypes.hpp>
#include <Platform/PC/Drivers/RTC/PCRTCDriverTypes.hpp>
#include <Platform/PC/Drivers/Serial/PCSerialDriverTypes.hpp>
#include <Platform/PC/Drivers/Timers/PCTimerDriverTypes.hpp>

namespace Quantum::Kernel::Platform::PC::Bootstrap {
  #if defined(ARCH_IA32)
  using namespace Quantum::Kernel::Arch::IA32;
  using namespace Quantum::Kernel::Arch::IA32::Concurrency;
  using namespace Quantum::Kernel::Arch::IA32::Drivers::CPU;
  using namespace Quantum::Kernel::Arch::IA32::Handlers;
  using namespace Quantum::Kernel::Arch::IA32::Interrupts;
  using namespace Quantum::Kernel::Arch::IA32::Memory;
  #endif

  using namespace Quantum::Kernel::Platform::PC::Drivers::Bus;
  using namespace Quantum::Kernel::Platform::PC::Drivers::DMA;
  using namespace Quantum::Kernel::Platform::PC::Drivers::Graphics;
  using namespace Quantum::Kernel::Platform::PC::Drivers::Interrupts;
  using namespace Quantum::Kernel::Platform::PC::Drivers::RTC;
  using namespace Quantum::Kernel::Platform::PC::Drivers::Serial;
  using namespace Quantum::Kernel::Platform::PC::Drivers::Timers;
}
