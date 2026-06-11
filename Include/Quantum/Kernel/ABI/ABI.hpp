/**
 * @file Include/Quantum/Kernel/ABI/ABI.hpp
 * @brief Declaration of the kernel's ABI namespace.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>
#include <Quantum/Kernel/KernelOperation.hpp>

namespace Quantum::Kernel::ABI {
  /**
   * @brief Invokes the kernel's ABI.
   * @param operation The ABI operation to perform.
   * @param arg1 First argument for the ABI operation.
   * @param arg2 Second argument for the ABI operation.
   * @param arg3 Third argument for the ABI operation.
   * @return The result of the ABI operation.
   */
  extern UInt32 Invoke(
    KernelOperation operation,
    UInt32 arg1 = 0,
    UInt32 arg2 = 0,
    UInt32 arg3 = 0
  );
}
