//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// System/Kernel/Include/Arch/IO.hpp
// Declarations for architecture‐specific I/O port operations.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#pragma once

#include <Arch/x86/IO.hpp>

namespace Quantum::Kernel::Arch::IO {
  using Quantum::Kernel::Arch::x86::IO::OutByte;
  using Quantum::Kernel::Arch::x86::IO::InByte;
  using Quantum::Kernel::Arch::x86::IO::OutWord;
  using Quantum::Kernel::Arch::x86::IO::InWord;
  using Quantum::Kernel::Arch::x86::IO::OutDword;
  using Quantum::Kernel::Arch::x86::IO::InDword;
}
