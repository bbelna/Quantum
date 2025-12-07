//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// System/Kernel/Kernel.cpp
// The core kernel implementation for Quantum.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#include <Kernel.hpp>
#include <Drivers/Console.hpp>

using namespace Quantum::Kernel;
using namespace Quantum::Kernel::Drivers;

void Kernel::Initialize() {
	Console::Initialize();
	Console::WriteString("Quantum\n");

  // TODO: kernel stuff

  Console::WriteString("System halted\n");
  while (true) {
    asm volatile("hlt");
  }
}
