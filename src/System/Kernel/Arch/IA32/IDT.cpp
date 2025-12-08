//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/IDT.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// IA32 kernel Interrupt Descriptor Table (IDT) setup.
//------------------------------------------------------------------------------

#include <Arch/IA32/IDT.hpp>
#include <Arch/IA32/InterruptContext.hpp>
#include <Arch/IA32/Drivers/VGAConsole.hpp>

using Quantum::Kernel::Arch::IA32::Drivers::VGAConsole;

namespace Quantum::Kernel::Arch::IA32 {

  static IDTEntry idtEntries[256];
  static IDTDescriptor idtDescriptor;
  static InterruptHandler handlerTable[256] = { nullptr };

  extern "C" void ISR0();
  extern "C" void LoadIDT(IDTDescriptor*);

  // NOTE: this is *internal* helper; not declared in the header.
  static void SetIDTGate(uint8 vector, void (*stub)()) {
    uint32 addr = reinterpret_cast<uint32>(stub);
    IDTEntry& e = idtEntries[vector];

    e.offsetLow = addr & 0xFFFF;
    e.selector = 0x08; // code segment selector
    e.zero = 0;
    e.typeAttribute = 0x8E; // present, ring 0, 32-bit interrupt gate
    e.offsetHigh = (addr >> 16) & 0xFFFF;
  }

  void InitializeIDT() {
    // zero entire table and handler table
    for (int i = 0; i < 256; ++i) {
      idtEntries[i] = IDTEntry{};
      handlerTable[i] = nullptr;
    }

    // set exception 0 (divide-by-zero) to go to ISR0 stub
    SetIDTGate(0, ISR0);

    idtDescriptor.limit = sizeof(idtEntries) - 1;
    idtDescriptor.base = reinterpret_cast<uint32>(&idtEntries[0]);

    LoadIDT(&idtDescriptor);

    VGAConsole::WriteLine("IDT initialized");
  }

  void SetIDTHandler(uint8 vector, InterruptHandler handler) {
    handlerTable[vector] = handler;
  }

  extern "C" void IDTExceptionHandler(uint32 vector, uint32 errorCode) {
    InterruptContext ctx{
      .vector    = static_cast<uint8>(vector),
      .errorCode = errorCode,
    };

    if (handlerTable[ctx.vector]) {
      handlerTable[ctx.vector](ctx);
    } else {
      VGAConsole::WriteLine("Unhandled interrupt vector");
      for (;;) {
        asm volatile("hlt");
      }
    }
  }
}
