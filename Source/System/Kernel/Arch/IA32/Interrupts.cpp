/**
 * @file System/Kernel/Arch/IA32/Interrupts.cpp
 * @brief IA32 interrupt handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "Arch/IA32/CPU.hpp"
#include "Arch/IA32/Exceptions.hpp"
#include "Arch/IA32/IDT.hpp"
#include "Arch/IA32/Interrupts.hpp"
#include "Arch/IA32/PIC.hpp"
#include "Arch/IA32/PS2Keyboard.hpp"
#include "Arch/IA32/SystemCalls.hpp"
#include "Arch/IA32/Timer.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  void Interrupts::Initialize() {
    IDT::Initialize();
    Exceptions::InstallDefaultHandlers();

    SystemCalls::Initialize();

    Timer::Initialize();
    PS2Keyboard::Initialize();

    CPU::EnableInterrupts();
  }

  void Interrupts::RegisterHandler(UInt8 vector, Interrupts::Handler handler) {
    IDT::SetHandler(vector, handler);
  }

  void Interrupts::End(UInt8 irq) {
    PIC::SendEOI(irq);
  }

  void Interrupts::Mask(UInt8 irq) {
    PIC::Mask(irq);
  }

  void Interrupts::MaskAll() {
    PIC::MaskAll();
  }

  void Interrupts::Unmask(UInt8 irq) {
    PIC::Unmask(irq);
  }

  void Interrupts::UnmaskAll() {
    PIC::UnmaskAll();
  }
}
