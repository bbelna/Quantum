//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/IDT.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// IA32 kernel Interrupt Descriptor Table (IDT) setup.
//------------------------------------------------------------------------------

#include <Arch/IA32/IDT.hpp>
#include <Arch/IA32/InterruptContext.hpp>
#include <Arch/IA32/Drivers/PIC.hpp>
#include <Drivers/Console.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  using Drivers::PIC;
  using Quantum::Kernel::Drivers::Console;

  constexpr UInt8 exceptionCount = 32;
  constexpr UInt8 irqBase = 32;
  constexpr UInt8 irqCount = 16;

  static IDTEntry idtEntries[256];
  static IDTDescriptor idtDescriptor;
  static InterruptHandler handlerTable[256] = { nullptr };

  extern "C" void ISR0();
  extern "C" void ISR1();
  extern "C" void ISR2();
  extern "C" void ISR3();
  extern "C" void ISR4();
  extern "C" void ISR5();
  extern "C" void ISR6();
  extern "C" void ISR7();
  extern "C" void ISR8();
  extern "C" void ISR9();
  extern "C" void ISR10();
  extern "C" void ISR11();
  extern "C" void ISR12();
  extern "C" void ISR13();
  extern "C" void ISR14();
  extern "C" void ISR15();
  extern "C" void ISR16();
  extern "C" void ISR17();
  extern "C" void ISR18();
  extern "C" void ISR19();
  extern "C" void ISR20();
  extern "C" void ISR21();
  extern "C" void ISR22();
  extern "C" void ISR23();
  extern "C" void ISR24();
  extern "C" void ISR25();
  extern "C" void ISR26();
  extern "C" void ISR27();
  extern "C" void ISR28();
  extern "C" void ISR29();
  extern "C" void ISR30();
  extern "C" void ISR31();
  extern "C" void IRQ0();
  extern "C" void IRQ1();
  extern "C" void IRQ2();
  extern "C" void IRQ3();
  extern "C" void IRQ4();
  extern "C" void IRQ5();
  extern "C" void IRQ6();
  extern "C" void IRQ7();
  extern "C" void IRQ8();
  extern "C" void IRQ9();
  extern "C" void IRQ10();
  extern "C" void IRQ11();
  extern "C" void IRQ12();
  extern "C" void IRQ13();
  extern "C" void IRQ14();
  extern "C" void IRQ15();
  extern "C" void LoadIDT(IDTDescriptor*);

  static void (*const exceptionStubs[exceptionCount])() = {
    ISR0,  ISR1,  ISR2,  ISR3,
    ISR4,  ISR5,  ISR6,  ISR7,
    ISR8,  ISR9,  ISR10, ISR11,
    ISR12, ISR13, ISR14, ISR15,
    ISR16, ISR17, ISR18, ISR19,
    ISR20, ISR21, ISR22, ISR23,
    ISR24, ISR25, ISR26, ISR27,
    ISR28, ISR29, ISR30, ISR31
  };

  static void (*const irqStubs[irqCount])() = {
    IRQ0,  IRQ1,  IRQ2,  IRQ3,
    IRQ4,  IRQ5,  IRQ6,  IRQ7,
    IRQ8,  IRQ9,  IRQ10, IRQ11,
    IRQ12, IRQ13, IRQ14, IRQ15
  };

  // NOTE: this is *internal* helper; not declared in the header.
  static void SetIDTGate(UInt8 vector, void (*stub)()) {
    UInt32 addr = reinterpret_cast<UInt32>(stub);
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

    for (UInt8 i = 0; i < exceptionCount; ++i) {
      SetIDTGate(i, exceptionStubs[i]);
    }

    for (UInt8 i = 0; i < irqCount; ++i) {
      SetIDTGate(irqBase + i, irqStubs[i]);
    }

    idtDescriptor.limit = sizeof(idtEntries) - 1;
    idtDescriptor.base = reinterpret_cast<UInt32>(&idtEntries[0]);

    LoadIDT(&idtDescriptor);

    PIC::Initialize(irqBase, irqBase + 8);
    PIC::MaskAll();
  }

  void SetIDTHandler(UInt8 vector, InterruptHandler handler) {
    handlerTable[vector] = handler;
  }

  extern "C" void IDTExceptionHandler(InterruptContext* ctx) {
    UInt8 vector = static_cast<UInt8>(ctx->vector);
    bool isIRQ = vector >= irqBase && vector < (irqBase + irqCount);
    bool spurious = !handlerTable[vector] &&
                    (vector == static_cast<UInt8>(irqBase + 7) ||
                     vector == static_cast<UInt8>(irqBase + 15));
    bool handled = handlerTable[vector] != nullptr;

    if (handled) {
      handlerTable[vector](*ctx);
    } else if (isIRQ && !spurious) {
      Console::WriteLine("Unhandled IRQ");
    } else {
      Console::WriteLine("Unhandled interrupt vector");
      for (;;) {
        asm volatile("hlt");
      }
    }

    if (isIRQ) {
      PIC::SendEOI(vector - irqBase);
    }
  }
}
