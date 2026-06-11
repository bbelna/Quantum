/**
 * @file Kernel/Core/IKernelModule.hpp
 * @brief Declares @ref @QKrnl::Core::IKernelModule.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "KernelModuleID.hpp"
#include "KernelModuleState.hpp"

namespace Quantum::Kernel {
  struct KernelContext;
}

namespace Quantum::Kernel::Core {
  /**
   * @brief Maximum number of dependencies a single module can declare.
   */
  constexpr Size MaxModuleDependencies = 16;

  /**
   * @brief Abstract interface for kernel modules.
   *
   * Every kernel subsystem that participates in the module system
   * implements this interface. The @ref KernelModuleRepository calls
   * @ref GetID and @ref GetDependencies to build the dependency graph,
   * then calls @ref Initialize in topological order.
   */
  class IKernelModule {
    public:
      virtual ~IKernelModule() = default;

      /**
       * @brief Returns the unique identifier for this module.
       */
      virtual KernelModuleID GetID() const = 0;

      /**
       * @brief Fills @p dependencies with the IDs of modules this
       *        module requires, and sets @p count to the number
       *        of dependencies.
       * @param dependencies Array of at least @ref MaxModuleDependencies
       *                     elements to fill.
       * @param count Set to the number of dependencies written.
       */
      virtual void GetDependencies(
        KernelModuleID* dependencies,
        Size& count
      ) const = 0;

      /**
       * @brief Initializes this module.
       * @param context Pointer to the @ref KernelContext. All dependencies
       *                declared by @ref GetDependencies are guaranteed
       *                to be initialized and their pointers populated
       *                in the context before this is called.
       * @return `true` on success, `false` on failure.
       */
      virtual bool Initialize(KernelContext* context) = 0;

      /**
       * @brief Returns the current lifecycle state of this module.
       */
      KernelModuleState GetState() const { return _state; }

      /**
       * @brief Sets the module lifecycle state.
       * @param state The new @ref KernelModuleState.
       */
      void SetState(KernelModuleState state) { _state = state; }

    private:
      KernelModuleState _state = KernelModuleState::Registered;
  };
}
