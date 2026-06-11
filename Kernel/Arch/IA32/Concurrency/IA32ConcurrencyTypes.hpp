/**
 * @file Kernel/Arch/IA32/Concurrency/IA32ConcurrencyTypes.hpp
 * @brief Declares @QKrnlIA32::Concurrency types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Kernel::Arch::IA32::Concurrency {
  class IA32TaskStateSegmentManager;

  template <typename ValueType>
  class IA32Atomic;
}

using namespace Quantum::Kernel::Arch::IA32::Concurrency;
