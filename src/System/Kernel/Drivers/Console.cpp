//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Drivers/Console.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// The kernel's console driver.
//------------------------------------------------------------------------------

#include <Drivers/Console.hpp>

#if defined(QUANTUM_ARCH_IA32)
  #include <Arch/IA32/Drivers/VGAConsole.hpp>

  namespace ConsoleNs = Quantum::Kernel::Arch::IA32::Drivers;
#else
  #error "No architecture selected for console driver"
#endif

namespace Quantum::Kernel::Drivers {
  using ConsoleDriver = ConsoleNs::VGAConsole;

  void Console::Initialize() {
    ConsoleDriver::Initialize();
  }

  void Console::WriteChar(char c) {
    ConsoleDriver::WriteChar(c);
  }

  void Console::Write(const char* str) {
    ConsoleDriver::Write(str);
  }

  void Console::WriteLine(const char* str) {
    ConsoleDriver::WriteLine(str);
  }

  void Console::WriteHex32(uint32 value) {
    ConsoleDriver::WriteHex32(value);
  }
}
