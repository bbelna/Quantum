/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/Types/InitBundleInfo.hpp
 * INIT.BND bundle info shared by kernel and userland.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::ABI::Types {
  /**
   * INIT.BND bundle info.
   */
  struct InitBundleInfo {
    /**
     * Virtual address of the bundle mapping in user space.
     */
    UInt32 Base;

    /**
     * Size of the bundle in bytes.
     */
    UInt32 Size;
  };
}
