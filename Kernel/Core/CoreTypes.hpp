/**
 * @file Kernel/Core/CoreTypes.hpp
 * @brief Declares @ref @QKrnl::Core types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Core {
  struct IntrusiveFreeNode;
  struct KernelModuleID;

  enum class KernelModuleState : UInt8;

  class IKernelModule;
  class KernelModuleRepository;
}

using namespace Quantum::Kernel::Core;
