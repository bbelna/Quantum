/**
 * @file Include/Quantum/ABI.hpp
 * @brief Includes all headers for, and declares, @ref @QABI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "ABI/ABIRequest.hpp"
#include "ABI/ABIRequestWithReplyPort.hpp"
#include "ABI/ABITypes.hpp"

/**
 * @brief Application Binary Interface (ABI) library.
 */
namespace Quantum::ABI {}

namespace QABI = Quantum::ABI;

template <typename Operation>
using ABIRequest = QABI::ABIRequest<Operation>;

template <typename Operation>
using ABIRequestWithReplyPort = QABI::ABIRequestWithReplyPort<Operation>;

using IPCPortID = QABI::IPCPortID;
