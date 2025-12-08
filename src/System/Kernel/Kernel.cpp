//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Kernel.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// The core kernel implementation for Quantum.
//------------------------------------------------------------------------------

#include <Kernel.hpp>
#include <Interrupts.hpp>
#include <Memory.hpp>
#include <Drivers/Console.hpp>

namespace Quantum::Kernel {
  using namespace Drivers;

  void Kernel::Initialize() {
    Console::Initialize();
    Console::WriteLine("Quantum");

    Memory::Initialize();
    Interrupts::Initialize();
  }
}
