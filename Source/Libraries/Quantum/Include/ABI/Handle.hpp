/**
 * @file Libraries/Quantum/Include/ABI/Handle.hpp
 * @brief Handle syscall wrappers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright Ac 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "ABI/SystemCall.hpp"
#include "Types.hpp"

namespace Quantum::ABI {
  /**
   * Handle syscall wrappers.
   */
  class Handle {
    public:
      /**
       * Handle information payload.
       */
      struct Info {
        UInt32 type;
        UInt32 rights;
      };

      /**
       * Closes a handle.
       * @param handle
       *   Handle to close.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Close(UInt32 handle) {
        return InvokeSystemCall(SystemCall::Handle_Close, handle, 0, 0);
      }

      /**
       * Duplicates a handle with optional rights mask.
       * @param handle
       *   Handle to duplicate.
       * @param rights
       *   Rights mask (0 to keep original rights).
       * @return
       *   New handle on success; 0 on failure.
       */
      static UInt32 Dup(UInt32 handle, UInt32 rights = 0) {
        return InvokeSystemCall(SystemCall::Handle_Dup, handle, rights, 0);
      }

      /**
       * Queries handle metadata.
       * @param handle
       *   Handle to query.
       * @param outInfo
       *   Receives handle info.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Query(UInt32 handle, Info& outInfo) {
        return InvokeSystemCall(
          SystemCall::Handle_Query,
          handle,
          reinterpret_cast<UInt32>(&outInfo),
          0
        );
      }
  };
}
