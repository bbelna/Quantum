/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/Interrupts/InterruptHandler.hpp
 * Signature for interrupt handler functions.
 */

#pragma once

#include <Types/Interrupts/InterruptContext.hpp>

namespace Quantum::System::Kernel::Types::Interrupts {
  /**
   * An interrupt handler function.
   * @param context
   *   Reference to the interrupt context.
   */
  typedef void(*InterruptHandler)(InterruptContext& context);
}
