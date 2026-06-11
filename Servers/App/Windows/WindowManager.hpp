/**
 * @file Servers/App/Windows/WindowManager.hpp
 * @brief Declares @ref @QAppSrv::Manager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Theme.hpp>

#include <AppServerTypes.hpp>

#include "Windows/Window.hpp"

namespace Quantum::Servers::App::Windows {
  /**
   * @brief Simple pair type used by WindowManager lookup methods.
   * @tparam A The type of the first element.
   * @tparam B The type of the second element.
   */
  template<typename A, typename B>
  struct Pair {
    /**
     * @brief The first element.
     */
    A First;

    /**
     * @brief The second element.
     */
    B Second;
  };

  /**
   * @brief Maps a window resource ID to its internal `Window*` pointer
   *        and list node.
   */
  struct WindowResource {
    /**
     * @brief The unique resource ID for this window, used by clients to
     *        refer to the window in IPC messages. Assigned by the server
     *        when the window is created.
     */
    UInt32 ID;

    /**
     * @brief The process ID that created this window.
     */
    UInt32 OwnerProcessID = 0;

    /**
     * @brief Pointer to the window instance.
     */
    Window* Ptr;

    /**
     * @brief Pointer to the list node of the window in the window list,
     *        used for efficient reordering during focus changes and Z-order
     *        updates.
     */
    PathNode<Window*>* Node;

    /**
     * @brief The IPC port ID of the client's reply port for close events.
     *        When a window is closed, a message is sent to this port to
     *        notify the client. This allows the client to clean up any
     *        resources associated with the window before it is destroyed.
     */
    Kernel::IPC::IPCPortID CloseReplyPortID;

    /**
     * @brief The shared buffer ID for the window's content pixel data.
     */
    Kernel::Memory::SharedBufferID ContentBufferID;

    /**
     * @brief Pointer to the mapped content buffer in the server's address
     *        space.
     */
    void* ContentBuffer;

    /**
     * @brief Row stride of the content buffer in pixels.
     */
    UInt16 ContentStride;

    /**
     * @brief The IPC port ID of the client's reply port for window events.
     */
    Kernel::IPC::IPCPortID EventReplyPortID;

    /**
     * @brief The minimum width of the window's content area in pixels,
     *        as requested by the client. Used to enforce size constraints
     *        during user-initiated resizing.
     */
    UInt16 MinWidth;

    /**
     * @brief The minimum height of the window's content area in pixels,
     *        as requested by the client. Used to enforce size constraints
     *        during user-initiated resizing.
     */
    UInt16 MinHeight;

    /**
     * @brief Whether a resize operation is currently pending for the
     *        window.
     */
    bool PendingResize;

    /**
     * @brief The new content width in pixels for a pending resize
     *        operation.
     */
    UInt16 PendingContentWidth;

    /**
     * @brief The new content height in pixels for a pending resize
     *        operation.
     */
    UInt16 PendingContentHeight;

    /**
     * @brief The last content width sent to the client.
     */
    UInt16 LastSentContentWidth = 0;

    /**
     * @brief The last content height sent to the client.
     */
    UInt16 LastSentContentHeight = 0;

    /**
     * @brief The capacity of the event queue for this window.
     */
    static constexpr Size EventQueueCapacity = 16;

    /**
     * @brief Fixed-size circular queue of pending events for this window.
     *        Used to buffer keyboard and scroll events when the client is
     *        not actively waiting for them via `GetNextEvent`, ensuring no
     *        input events are lost during periods of inactivity or slow
     *        response.
     */
    ABI::WindowEventResult EventQueue[EventQueueCapacity];

    /**
     * @brief The index of the head of the event queue (the oldest event).
     */
    Size EventQueueHead = 0;

    /**
     * @brief The number of events currently in the event queue.
     */
    Size EventQueueCount = 0;

    /**
     * @brief Resource ID of the modal dialog blocking this window, or
     *        0 if the window is not blocked. When non-zero, clicks on
     *        this window are redirected to bring the dialog to front.
     */
    UInt32 ModalDialogID = 0;

    /**
     * @brief Delivers an event to the window's client, either immediately
     *        (if blocking in GetWindowEvent) or by enqueuing.
     * @param event The event to deliver.
     * @param kernel Reference to the kernel client for IPC operations.
     * @return `true` if the event was delivered or enqueued.
     */
    bool DeliverEvent(
      const ABI::WindowEventResult& event,
      KernelClient& kernel
    ) {
      if (EventReplyPortID != 0) {
        IPCPortResourceID rh = kernel.OpenIPCPort(
          EventReplyPortID, IPCPortRights::Send
        );

        if (rh != static_cast<IPCPortResourceID>(-1)) {
          kernel.SendIPCMessage(rh, &event, sizeof(event));
          kernel.CloseIPCPort(rh);
        }

        EventReplyPortID = 0;

        return true;
      }

      if (EventQueueCount < EventQueueCapacity) {
        Size tail = (EventQueueHead + EventQueueCount) % EventQueueCapacity;
        EventQueue[tail] = event;
        EventQueueCount++;

        return true;
      }

      return false;
    }
  };

  /**
   * @brief Manages window lifecycle, Z-order, focus, and event delivery.
   *
   * The WindowManager owns the Z-ordered window list, the per-window
   * resource table, and the deferred-deletion queue.  It provides methods
   * for bringing windows to front, activating / deactivating focus, and
   * looking up resources by ID or pointer.
   */
  class WindowManager {
    public:
      /**
       * @brief Maximum number of windows tracked via the resource table.
       */
      static constexpr Size MaxWindows = 64;

      /**
       * @brief The 32-bit ARGB color value for the desktop background.
       *        Used when damaging the area vacated by a closed window.
       */
      static constexpr UInt32 BackgroundColor = Theme::DesktopBackground;

      /**
       * @brief Creates a new WindowManager instance.
       * @param kernel Reference to the kernel client for IPC operations.
       * @param log Reference to the server log for logging.
       */
      explicit WindowManager(KernelClient& kernel, ServerLog& log)
        : _kernel(kernel), _log(log) {}

      /**
       * @brief Returns the topmost (frontmost) window node whose bounds
       *        contain the given point, or `nullptr` if no window is at
       *        that position.
       * @param p The point to test.
       * @return Pointer to the matching list node, or `nullptr`.
       */
      PathNode<Window*>* FindWindowNodeAt(Point p) const;

      /**
       * @brief Searches the resource table for a window with the given ID.
       * @param id The window resource ID to search for.
       * @return A pair of the index into the resource table and a pointer
       *         to the matching resource, or `{_windowResourceCount,
       *         nullptr}` if not found.
       */
      Pair<Size, WindowResource*> FindResourceByID(UInt32 id);

      /**
       * @brief Searches the resource table for a resource matching the
       *        given window pointer.
       * @param w The window pointer to search for.
       * @return A pair of the index into the resource table and a pointer
       *         to the matching resource, or `{_windowResourceCount,
       *         nullptr}` if not found.
       */
      Pair<Size, WindowResource*> FindResourceByWindow(Window* w);

      /**
       * @brief Removes the given window node from its current position in
       *        the Z-order list and appends it at the tail (frontmost).
       * @param node The window list node to bring to front.
       */
      void BringToFront(PathNode<Window*>* node);

      /**
       * @brief Sets the given window node as the active (focused) window.
       * @param node The window list node to activate.
       */
      void ActivateWindow(PathNode<Window*>* node);

      /**
       * @brief Deactivates the currently active window, if any, sending a
       *        Deactivated event to its client.
       */
      void DeactivateCurrentWindow();

      /**
       * @brief Sends a `Deactivated` event to the window at the given
       *        node, either immediately or via its event queue.
       * @param node The window node being deactivated.
       */
      void SendDeactivatedEvent(PathNode<Window*>* node);

      /**
       * @brief If the given window is a modal dialog, ensures its parent
       *        window is positioned directly below it in the Z-order.
       * @param dialogNode The window node that was just brought to front.
       * @return The parent node if it was moved, or `nullptr`.
       */
      PathNode<Window*>* EnsureDialogParentBelow(
        PathNode<Window*>* dialogNode
      );

      /**
       * @brief Adds a window to the deferred-deletion queue.
       * @param w Pointer to the window to delete later.
       */
      void QueueDeleteWindow(Window* w);

      /**
       * @brief Deletes all windows currently in the deferred-deletion
       *        queue and resets the queue count to zero.
       */
      void ProcessDeleteQueue();

      /**
       * @brief Returns the currently active (focused) window node.
       * @return Pointer to the active list node, or `nullptr` if no
       *         window is active.
       */
      PathNode<Window*>* GetActiveNode() const;

      /**
       * @brief Returns a mutable reference to the Z-ordered window list.
       * @return Reference to the window list (head = backmost, tail =
       *         frontmost).
       */
      List<Window*>& GetWindows();

      /**
       * @brief Returns the number of active window resources.
       * @return Current count of entries in the resource table.
       */
      Size GetResourceCount() const;

      /**
       * @brief Returns a mutable reference to the resource at the given
       *        index.
       * @param index Zero-based index into the resource table.
       * @return Reference to the window resource.
       */
      WindowResource& GetResource(Size index);

      /**
       * @brief Allocates a new slot in the resource table and increments
       *        the count.
       * @return The index of the newly allocated slot.
       */
      Size AllocateResource();

      /**
       * @brief Frees the resource at the given index by compacting the
       *        table (shifting subsequent entries down by one).
       * @param index Zero-based index of the resource to free.
       */
      void FreeResource(Size index);

      /**
       * @brief Returns the next window resource ID and increments the
       *        internal counter.
       * @return The allocated resource ID.
       */
      UInt32 AllocateResourceID();

      /**
       * @brief Sets the active window node directly.
       * @param node The new active node, or `nullptr` to clear.
       * @param deferContextUpdate If true, skip the focus context IPC
       *        to the context server. The caller is responsible for
       *        sending it later (e.g., on first content invalidation).
       */
      void SetActiveNode(
        PathNode<Window*>* node,
        bool deferContextUpdate = false
      );

      /**
       * @brief Sends the focus context update IPC for the current active
       *        node. Used to flush a deferred context update after the
       *        window's content is ready.
       */
      void FlushFocusContext();

    private:
      /**
       * @brief Reference to the kernel client for IPC operations.
       */
      KernelClient& _kernel;

      /**
       * @brief Reference to the server log.
       */
      ServerLog& _log;

      /**
       * @brief Client for context server focus updates.
       */
      ContextClient _contextClient;

      /**
       * @brief The ordered list of windows. Head is backmost, tail is
       *        frontmost (topmost in Z-order).
       */
      List<Window*> _windows;

      /**
       * @brief The list node of the currently active (focused) window, or
       *        `nullptr` if no window is active.
       */
      PathNode<Window*>* _activeNode = nullptr;

      /**
       * @brief Fixed-size lookup table of active window resources.
       */
      WindowResource _windowResources[MaxWindows] = {};

      /**
       * @brief Number of entries in `_windowResources`.
       */
      Size _windowResourceCount = 0;

      /**
       * @brief Monotonically increasing counter for assigning window
       *        resource IDs to newly created windows.
       */
      UInt32 _nextWindowResourceID = 1;

      /**
       * @brief The delete queue for windows that have been closed but not
       *        yet destroyed.
       *
       * When a window is closed, it is removed from the active window
       * list and added to this queue. The render thread checks this queue
       * after each render cycle and destroys any windows in it, ensuring
       * that windows are not destroyed while they may still be referenced
       * during rendering.
       */
      Window* _deleteQueue[MaxWindows] = {};

      /**
       * @brief The number of windows currently in the delete queue.
       */
      Size _deleteQueueCount = 0;
  };
}
