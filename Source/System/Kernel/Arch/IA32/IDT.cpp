/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/IDT.cpp
 * IA32 Interrupt Descriptor Table management.
 */

#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Drivers/PIC.hpp>
#include <Arch/IA32/IDT.hpp>
#include <Arch/IA32/Types/IDT/IDTEntry.hpp>
#include <Arch/IA32/Types/IDT/IDTDescriptor.hpp>
#include <Arch/IA32/Types/IDT/InterruptContext.hpp>
#include <Logger.hpp>
#include <Types/Logging/Level.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using namespace Types::IDT;

  using InterruptHandler
    = Quantum::System::Kernel::Types::Interrupts::InterruptHandler;
  using LogLevel = Quantum::System::Kernel::Types::Logging::Level;
  using PIC = Drivers::PIC;

  namespace {
    /**
     * Total number of ISR exceptions.
     */
    constexpr UInt8 _exceptionCount = 32;

    /**
     * IRQ base vector.
     */
    constexpr UInt8 _irqBase = 32;

    /**
     * Total number of IRQs.
     */
    constexpr UInt8 _irqCount = 16;

    /**
     * IDT entries array.
     */
    static IDTEntry _idtEntries[256];

    /**
     * IDT descriptor for `lidt` instruction.
     */
    static IDTDescriptor _idtDescriptor;

    /**
     * Interrupt handler table.
     */
    static InterruptHandler _handlerTable[256] = { nullptr };
  }

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

  static void (*const exceptionStubs[_exceptionCount])() = {
    ISR0,  ISR1,  ISR2,  ISR3,
    ISR4,  ISR5,  ISR6,  ISR7,
    ISR8,  ISR9,  ISR10, ISR11,
    ISR12, ISR13, ISR14, ISR15,
    ISR16, ISR17, ISR18, ISR19,
    ISR20, ISR21, ISR22, ISR23,
    ISR24, ISR25, ISR26, ISR27,
    ISR28, ISR29, ISR30, ISR31
  };

  static void (*const irqStubs[_irqCount])() = {
    IRQ0,  IRQ1,  IRQ2,  IRQ3,
    IRQ4,  IRQ5,  IRQ6,  IRQ7,
    IRQ8,  IRQ9,  IRQ10, IRQ11,
    IRQ12, IRQ13, IRQ14, IRQ15
  };

  static void SetIDTGate(UInt8 vector, void (*stub)()) {
    UInt32 addr = reinterpret_cast<UInt32>(stub);
    IDTEntry& e = _idtEntries[vector];

    e.OffsetLow = addr & 0xFFFF;
    e.Selector = 0x08; // code segment selector
    e.Zero = 0;
    e.TypeAttribute = 0x8E; // present, ring 0, 32-bit interrupt gate
    e.OffsetHigh = (addr >> 16) & 0xFFFF;
  }

  void IDT::Initialize() {
    // zero entire table and handler table
    for (int i = 0; i < 256; ++i) {
      _idtEntries[i] = IDTEntry{};
      _handlerTable[i] = nullptr;
    }

    for (UInt8 i = 0; i < _exceptionCount; ++i) {
      SetIDTGate(i, exceptionStubs[i]);
    }

    for (UInt8 i = 0; i < _irqCount; ++i) {
      SetIDTGate(_irqBase + i, irqStubs[i]);
    }

    _idtDescriptor.Limit = sizeof(_idtEntries) - 1;
    _idtDescriptor.Base = reinterpret_cast<UInt32>(&_idtEntries[0]);

    LoadIDT(&_idtDescriptor);

    PIC::Initialize(_irqBase, _irqBase + 8);
    PIC::MaskAll();
  }

  void IDT::SetHandler(UInt8 vector, InterruptHandler handler) {
    _handlerTable[vector] = handler;
  }

  void IDT::DispatchInterrupt(InterruptContext* ctx) {
    UInt8 vector = static_cast<UInt8>(ctx->Vector);
    bool isIRQ = vector >= _irqBase && vector < (_irqBase + _irqCount);
    bool spurious
      = !_handlerTable[vector] && (
        vector == static_cast<UInt8>(_irqBase + 7) ||
        vector == static_cast<UInt8>(_irqBase + 15)
      );
    bool handled = _handlerTable[vector] != nullptr;

    if (handled) {
      _handlerTable[vector](*ctx);
    } else if (isIRQ && !spurious) {
      Logger::Write(LogLevel::Error, "Unhandled IRQ");
    } else {
      Logger::Write(LogLevel::Error, "Unhandled interrupt vector");
      CPU::HaltForever();
    }

    if (isIRQ) {
      PIC::SendEOI(vector - _irqBase);
    }
  }

  extern "C" void IDTExceptionHandler(InterruptContext* ctx) {
    IDT::DispatchInterrupt(ctx);
  }
}
