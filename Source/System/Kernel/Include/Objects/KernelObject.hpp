/**
 * @file System/Kernel/Include/Objects/KernelObject.hpp
 * @brief Kernel object base.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Objects/KernelObjectType.hpp"

namespace Quantum::System::Kernel::Objects {
  /**
   * Kernel object.
   */
  class KernelObject {
    public:
      /**
       * Object type.
       */
      KernelObjectType type;

      /**
       * Constructs a kernel object.
       * @param type
       *   Kernel object type.
       */
      explicit KernelObject(KernelObjectType type = KernelObjectType::None);

      /**
       * Virtual destructor for polymorphic cleanup.
       */
      virtual ~KernelObject();

      /**
       * Increments the kernel object refcount.
       */
      void AddRef();

      /**
       * Releases a kernel object reference.
       */
      void Release();

    private:
      /**
       * Reference count.
       */
      UInt32 _refCount;
  };
}
