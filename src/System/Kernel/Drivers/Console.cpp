//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// System/Kernel/Drivers/Console.cpp
// The kernel's console driver.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#include <Drivers/Console.hpp>

// TODO: make this select backend based on architecture
// #if defined(QUANTUM_ARCH_IA32)
#include <Arch/IA32/Drivers/VgaConsole.hpp>

namespace ConsoleNs = Quantum::Kernel::Arch::IA32::Drivers;
// #endif

namespace Quantum::Kernel::Drivers {
  using ConsoleDriver = ConsoleNs::VgaConsole;

  void Console::Initialize() {
    ConsoleDriver::Initialize();
  }

  void Console::WriteChar(char c) {
    ConsoleDriver::WriteChar(c);
  }

  void Console::WriteString(const char* str) {
    ConsoleDriver::WriteString(str);
  }
}
