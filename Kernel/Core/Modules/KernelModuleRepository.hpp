/**
 * @file Kernel/Core/KernelModuleRepository.hpp
 * @brief Declares @ref @QKrnl::Core::KernelModuleRepository.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "IKernelModule.hpp"

namespace Quantum::Kernel::Core {
  /**
   * @brief Maximum number of kernel modules the repository can hold.
   */
  constexpr Size MaxKernelModules = 64;

  /**
   * @brief Repository that holds all kernel modules and initializes
   *        them in dependency order.
   *
   * Modules are registered via @ref Register, then @ref InitializeAll
   * performs a topological sort using Kahn's algorithm and calls
   * @ref IKernelModule::Initialize on each module in dependency order.
   */
  class KernelModuleRepository {
    public:
      KernelModuleRepository() = default;

      /**
       * @brief Registers a module with the repository.
       * @param module Pointer to the module (must outlive the repository).
       * @return `true` if registered, `false` if the repository is full.
       */
      bool Register(IKernelModule* module);

      /**
       * @brief Initializes all registered modules in dependency order.
       * @param context Pointer to the @ref KernelContext.
       * @return `true` if all modules initialized successfully.
       *
       * Performs a topological sort of the dependency graph using
       * Kahn's algorithm. If a cycle is detected, the method returns
       * `false`.
       */
      bool InitializeAll(KernelContext* context);

      /**
       * @brief Finds a registered module by its ID.
       * @param id The @ref KernelModuleID to search for.
       * @return Pointer to the module, or `nullptr` if not found.
       */
      IKernelModule* Find(const KernelModuleID& id) const;

      /**
       * @brief Returns the number of registered modules.
       */
      Size GetCount() const { return _count; }

    private:
      IKernelModule* _modules[MaxKernelModules] = {};
      Size _count = 0;

      /**
       * @brief Resolves the initialization order via topological sort.
       * @param order Output array filled with modules in init order.
       * @param orderCount Set to the number of modules in the order.
       * @return `true` if resolution succeeded (no cycles).
       */
      bool _resolveOrder(
        IKernelModule** order,
        Size& orderCount
      );

      /**
       * @brief Finds the index of a module by its ID.
       * @param id The @ref KernelModuleID to search for.
       * @return The index, or @ref _count if not found.
       */
      Size _findIndex(const KernelModuleID& id) const;
  };
}
