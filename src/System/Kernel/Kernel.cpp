//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// Kernel/Kernel.cpp
// The core kernel implementation for Quantum.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#include <Kernel.hpp>

using namespace Quantum::Kernel;
using namespace Quantum::Kernel::Drivers;

void Kernel::Start() {
	Console::Initialize();
	Console::WriteString("Quantum\n");
  while (true) {
    asm volatile("hlt");
  }
}
