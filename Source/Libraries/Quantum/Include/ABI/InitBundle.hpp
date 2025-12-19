/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/InitBundle.hpp
 * User-mode INIT.BND access helpers.
 */

#pragma once

#include <ABI/SystemCall.hpp>

namespace Quantum::ABI {
  /**
   * INIT bundle ABI helpers.
   */
  class InitBundle {
    public:
      /**
       * INIT.BND bundle info.
       */
      struct Info {
        /**
         * Virtual address of the bundle mapping in user space.
         */
        UInt32 base;

        /**
         * Size of the bundle in bytes.
         */
        UInt32 size;
      };

      /**
       * Retrieves INIT.BND bundle info from the kernel.
       * @param out
       *   Output structure populated by the kernel.
       * @return
       *   True if the bundle exists; false if not available.
       */
      static bool GetInfo(Info& out) {
        UInt32 result = InvokeSystemCall(
          SystemCall::GetInitBundleInfo,
          reinterpret_cast<UInt32>(&out)
        );

        return result == 0;
      }
  };
}
