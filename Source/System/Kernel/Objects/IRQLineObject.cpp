/**
 * @file System/Kernel/Objects/IRQLineObject.cpp
 * @brief IRQ line kernel object.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Objects/IRQLineObject.hpp"

namespace Quantum::System::Kernel::Objects {
  IRQLineObject::IRQLineObject(UInt32 irq)
    : KernelObject(KernelObjectType::IRQLine),
    irqLine(irq)
  {}
}
