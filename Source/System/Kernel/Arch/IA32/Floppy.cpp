/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Floppy.cpp
 * IA32 floppy controller interrupt handling.
 */

#include "Arch/IA32/Floppy.hpp"
#include "Arch/IA32/PIC.hpp"
#include "Devices/BlockDevice.hpp"
#include "Interrupts.hpp"
#include "Prelude.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using BlockDevice = Kernel::Devices::BlockDevice;

  Interrupts::Context* Floppy::IRQHandler(
    Interrupts::Context& context
  ) {
    BlockDevice::HandleFloppyIRQ();

    return &context;
  }

  void Floppy::Initialize() {
    Interrupts::RegisterHandler(38, IRQHandler); // IRQ6 vector
    PIC::Unmask(6);
  }
}
