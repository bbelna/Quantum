/**
 * @file Kernel/IPC/IPCPort.hpp
 * @brief Declares @ref @QKrnl::IPC::IPCPort.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Concurrency/Spinlock.hpp>
#include <Concurrency/ThreadWaitNode.hpp>
#include <KernelTypes.hpp>
#include <Resources/KernelResource.hpp>

namespace Quantum::Kernel::IPC {
  /**
   * @brief Structure representing an IPC port.
   *
   * Contains a unique @ref IPCPortID, a pointer to the owning @ref Process,
   * and queues for @ref IPCMessage and @ref Thread waiting to send/receive
   * @ref IPCMessage.
   */
  struct IPCPort {
    /**
     * @brief @ref IPCPortID for the @ref IPCPort.
     */
    IPCPortID ID;

    /**
     * @brief Pointer to the @ref Process that owns this @ref IPCPort.
     */
    Process* OwnerProcess;

    /**
     * @brief @ref Spinlock for synchronizing access to the @ref IPCPort data
     *        structures (e.g., message queue, send/receive queues, etc.).
     */
    Spinlock<UInt32> Lock;

    /**
     * @brief @ref IntrusiveQueue of @ref IPCMessage instances sent to the
     *        @ref IPCPort.
     */
    IntrusiveQueue<IPCMessage> MessageQueue;

    /**
     * @brief Maximum number of @ref IPCMessage instances that can be queued in
     *        the @ref IPCPort.
     */
    Size MaxMessageQueueSize = 1024;

    /**
     * @brief @ref IntrusiveQueue of @ref Thread instances waiting to send
     *        @ref IPCMessage instances to this @ref IPCPort.
     */
    IntrusiveQueue<ThreadWaitNode> SendQueue;

    /**
     * @brief @ref IntrusiveQueue of @ref Thread instances waiting to receive
     *        @ref IPCMessage instances from this @ref IPCPort.
     */
    IntrusiveQueue<ThreadWaitNode> ReceiveQueue;
  };

  /**
   * @brief Capability-style resource handle for @ref IPCPort.
   *
   * Wraps a @ref Resource pairing a unique resource ID (@ref IPCPortID) with a
   * set of @ref IPCPortRights that govern what operations the holder may
   * perform on the associated @ref IPCPort.
   */
  struct IPCPortResource : public KernelResource<IPCPortID, IPCPortRights> {};
}
