//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// System/Kernel/Include/Drivers/IO.hpp
// Declarations for I/O port operations.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#pragma once

#include <Arch/IA32/Drivers/IO.hpp>

namespace Quantum::Kernel::Drivers::IO {
  using Quantum::Kernel::Arch::IA32::Drivers::IO::OutByte;
  using Quantum::Kernel::Arch::IA32::Drivers::IO::InByte;
  using Quantum::Kernel::Arch::IA32::Drivers::IO::OutWord;
  using Quantum::Kernel::Arch::IA32::Drivers::IO::InWord;
  using Quantum::Kernel::Arch::IA32::Drivers::IO::OutDword;
  using Quantum::Kernel::Arch::IA32::Drivers::IO::InDword;
}
