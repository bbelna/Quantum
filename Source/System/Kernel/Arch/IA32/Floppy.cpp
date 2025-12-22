/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Floppy.cpp
 * IA32 floppy controller interrupt handling.
 */

#include <Arch/IA32/Floppy.hpp>
#include <Arch/IA32/PIC.hpp>
#include <Devices/Block.hpp>
#include <Interrupts.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using Block = ::Quantum::System::Kernel::Devices::Block;

  Interrupts::Context* Floppy::FloppyIRQHandler(
    Interrupts::Context& context
  ) {
    Block::HandleFloppyIRQ();

    return &context;
  }

  void Floppy::Initialize() {
    Interrupts::RegisterHandler(38, FloppyIRQHandler); // IRQ6 vector
    PIC::Unmask(6);
  }
}
