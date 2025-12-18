/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Kernel.hpp
 * Core kernel implementation.
 */

#pragma once

#include <Types/Primitives.hpp>
#include <Types/String.hpp>
#include <Types/Interrupts/InterruptContext.hpp>

#define PANIC(msg) \
  ::Quantum::System::Kernel::Panic(\
    (msg), __FILE__, __LINE__, __FUNCTION__\
  )

namespace Quantum::System::Kernel {
  using Types::String;
  using Types::Interrupts::InterruptContext;

  /**
   * Initializes the kernel.
   * @param bootInfoPhysicalAddress
   *   Physical address of the boot info block.
   */
  void Initialize(UInt32 bootInfoPhysicalAddress);

  /**
   * Handles a system call interrupt.
   * @param context
   *   Interrupt context at the time of the system call.
   * @return
   *   Updated interrupt context.
   */
  InterruptContext* HandleSystemCall(InterruptContext& context);

  /**
   * Retrieves the INIT.BND mapping for user space.
   * @param base
   *   Receives the user-space virtual base.
   * @param size
   *   Receives the bundle size in bytes.
   * @return
   *   True if INIT.BND is present and mapped; false otherwise.
   */
  bool GetInitBundleInfo(UInt32& base, UInt32& size);

  /**
   * Panic the kernel with a message.
   * Do not invoke directly. Use the macro `PANIC(msg)` as it captures
   * file, line, and function information automatically.
   * @param message
   *   Panic message.
   * @param file
   *   The source file where the panic occurred (optional).
   * @param line
   *   The line number where the panic occurred (optional).
   * @param function
   *   The function name where the panic occurred (optional).
   */
  void Panic(
    String message,
    String file = nullptr,
    UInt32 line = -1,
    String function = nullptr
  );
}
