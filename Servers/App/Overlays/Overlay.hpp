/**
 * @file Servers/App/Overlays/Overlay.hpp
 * @brief Declares the @ref @QAppSrv::Overlays::Overlay struct.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

#include "OverlayConstants.hpp"

namespace Quantum::Servers::App::Overlays {
  /**
   * @brief A system overlay surface rendered above all windows.
   *
   * Overlays never participate in the window focus/activation system.
   * The compositor draws them after all windows (back-to-front by
   * insertion order). The input dispatcher hit-tests them before
   * windows (front-to-back) and delivers mouse events via a reply
   * port.
   */
  struct Overlay {
    /**
     * @brief Unique overlay resource ID. 0 means this slot is unused.
     */
    UInt32 ID = 0;

    /**
     * @brief The overlay's screen-space frame rectangle.
     */
    Rectangle Frame;

    /**
     * @brief Pointer to the content buffer (shared memory, mapped into
     *        the AppServer's address space).
     */
    void* ContentBuffer = nullptr;

    /**
     * @brief Shared buffer ID for the content buffer.
     */
    Kernel::Memory::SharedBufferID ContentBufferID = 0;

    /**
     * @brief Content width in pixels.
     */
    UInt16 ContentWidth = 0;

    /**
     * @brief Content height in pixels.
     */
    UInt16 ContentHeight = 0;

    /**
     * @brief Content stride in pixels.
     */
    UInt16 ContentStride = 0;

    /**
     * @brief Bytes per pixel (2 for RGB565, 4 for ARGB32).
     */
    UInt8 ContentBytesPerPixel = 0;

    /**
     * @brief Reply port ID for the pending GetOverlayEvent request,
     *        or 0 if no request is pending.
     */
    Kernel::IPC::IPCPortID EventReplyPortID = 0;

    /**
     * @brief Circular event queue capacity.
     */
    static constexpr Size EventQueueCapacity = 8;

    /**
     * @brief Circular event queue for events that arrive before the
     *        client has re-requested.
     */
    ABI::WindowEventResult EventQueue[EventQueueCapacity] = {};

    /**
     * @brief Head index of the circular event queue.
     */
    Size EventQueueHead = 0;

    /**
     * @brief Number of queued events.
     */
    Size EventQueueCount = 0;

    /**
     * @brief Returns whether a point is within this overlay's frame.
     * @param x Screen-space X coordinate.
     * @param y Screen-space Y coordinate.
     */
    bool HitTest(Int16 x, Int16 y) const {
      return x >= Frame.Origin.X
        && x < Frame.Origin.X + static_cast<Int16>(Frame.Dimensions.Width)
        && y >= Frame.Origin.Y
        && y < Frame.Origin.Y + static_cast<Int16>(Frame.Dimensions.Height);
    }

    /**
     * @brief Delivers an event to the overlay's waiting client. Sends
     *        the event result to the reply port and clears it.
     * @param event The event to deliver.
     * @param kernel Reference to the kernel client for IPC operations.
     * @return `true` if the event was delivered.
     */
    bool DeliverEvent(
      const ABI::WindowEventResult& event,
      KernelClient& kernel
    );
  };
}
