/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/InitBundle.hpp
 * User-mode INIT.BND access helpers.
 */

#pragma once

#include <ABI/InvokeSystemCall.hpp>
#include <ABI/Types/InitBundleInfo.hpp>
#include <ABI/Types/SystemCall.hpp>

namespace Quantum::ABI {
  /**
   * INIT bundle ABI helpers.
   */
  class InitBundle {
    public:
      /**
       * Retrieves INIT.BND bundle info from the kernel.
       * @param out
       *   Output structure populated by the kernel.
       * @return
       *   True if the bundle exists; false if not available.
       */
      static bool GetInfo(Types::InitBundleInfo& out) {
        UInt32 result = InvokeSystemCall(
          Types::SystemCall::GetInitBundleInfo,
          reinterpret_cast<UInt32>(&out)
        );

        return result == 0;
      }
  };
}
