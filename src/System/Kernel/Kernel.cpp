//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// Kernel/Kernel.cpp
// The core kernel implementation for Quantum.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#include <Kernel.hpp>

using namespace Quantum;

void Kernel::Start() {
  while (true) {
    asm volatile("hlt");
  }
}
