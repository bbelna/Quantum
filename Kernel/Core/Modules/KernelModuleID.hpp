/**
 * @file Kernel/Core/KernelModuleID.hpp
 * @brief Declares @ref @QKrnl::Core::KernelModuleID.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Kernel::Core {
  /**
   * @brief Strongly-typed identifier for a kernel module.
   *
   * Each module declares a unique compile-time string identifier using
   * reverse-domain notation (e.g., `"Quantum.Kernel.Core"`). The
   * @ref KernelModuleRepository uses these to match declared dependencies
   * to registered modules.
   */
  struct KernelModuleID {
    /**
     * @brief The module identifier string.
     *
     * Must be a compile-time string literal. Compared by value
     * (character-by-character), not by pointer.
     */
    const char* Name;

    /**
     * @brief Compares two module identifiers for equality.
     * @param other The other @ref KernelModuleID to compare against.
     * @return `true` if both identifiers have the same name string.
     */
    bool operator==(const KernelModuleID& other) const {
      if (Name == other.Name) {
        return true;
      }

      if (Name && other.Name) {
        const char* a = Name;
        const char* b = other.Name;

        while (*a && *b) {
          if (*a != *b) {
            return false;
          }

          ++a;
          ++b;
        }

        return *a == *b;
      }

      return false;
    }

    /**
     * @brief Compares two module identifiers for inequality.
     * @param other The other @ref KernelModuleID to compare against.
     * @return `true` if the identifiers differ.
     */
    bool operator!=(const KernelModuleID& other) const {
      return !(*this == other);
    }
  };
}
