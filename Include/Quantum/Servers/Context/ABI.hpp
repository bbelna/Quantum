/**
 * @file Include/Quantum/Servers/Context/ABI.hpp
 * @brief Declares the context server's ABI types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/ABI.hpp>
#include <Quantum/Types.hpp>
#include <Quantum/Menus.hpp>
#include <Quantum/UI.hpp>

namespace Quantum::Servers::Context::ABI {
  /**
   * @brief ABI version for the context server protocol.
   */
  constexpr UInt32 ContextABIVersion = 1;

  /**
   * @brief IPC port ID for the context server.
   */
  constexpr IPCPortID ContextPortID = 9;

  /**
   * @brief Operations supported by the context server.
   */
  enum class ContextOperation : UInt32 {
    /**
     * @brief Registers a new menu provider and returns its provider ID.
     */
    RegisterProvider = 1,

    /**
     * @brief Unregisters a previously registered menu provider.
     */
    UnregisterProvider = 2,

    /**
     * @brief Pushes a new set of menu contributions from a provider.
     *        The contributions are passed via a shared buffer.
     */
    SetContributions = 3,

    /**
     * @brief Notifies the context server that the focus context has changed
     *        (sent by the application server on window focus change, or by
     *        an app on selection change).
     */
    UpdateFocusContext = 4,

    /**
     * @brief Dispatches a menu action invocation. Sent by the application
     *        server when the user clicks a menu item.
     */
    InvokeAction = 5,

    /**
     * @brief Retrieves the current composed menu bar state. The result is
     *        returned via a shared buffer.
     */
    GetMenuBarState = 6
  };

  /**
   * @brief Base request type for fire-and-forget context operations.
   */
  using ContextRequest = ABIRequest<ContextOperation>;

  /**
   * @brief Base request type for context operations that expect a reply.
   */
  using ContextRequestWithReply
    = ABIRequestWithReplyPort<ContextOperation>;

  /**
   * @brief Header for a single menu in the shared buffer wire format.
   *        Followed immediately by `ItemCount` @ref Menus::MenuItem
   *        structs.
   */
  struct WireMenuHeader {
    /**
     * @brief The unique menu identifier.
     */
    Menus::MenuID ID;

    /**
     * @brief The menu title.
     */
    char Title[Menus::MaxMenuTitleLength];

    /**
     * @brief The menu position in the bar.
     */
    UInt16 Position;

    /**
     * @brief Number of @ref Menus::MenuItem structs following this
     *        header.
     */
    UInt16 ItemCount;

    /**
     * @brief If true, this is a focus menu (rendered bold, sorted first).
     */
    bool IsFocusMenu;
  };

  /**
   * @brief Header at the start of a contribution shared buffer.
   *
   * Layout in the buffer:
   *   - @ref WireContributionHeader
   *   - `SuppressedCount` @ref Menus::ActionID values
   *   - For each menu: @ref WireMenuHeader + `ItemCount` MenuItem structs
   */
  struct WireContributionHeader {
    /**
     * @brief Number of menus in this contribution.
     */
    UInt16 MenuCount;

    /**
     * @brief Number of suppressed action IDs following this header.
     */
    UInt16 SuppressedCount;

    /**
     * @brief Total size of the wire data in bytes (including this header).
     */
    UInt32 TotalSize;
  };

  /**
   * @brief Header at the start of a menu bar state shared buffer.
   *
   * Layout in the buffer:
   *   - @ref WireMenuBarHeader
   *   - For each menu: @ref WireMenuHeader + `ItemCount` MenuItem structs
   */
  struct WireMenuBarHeader {
    /**
     * @brief Number of menus in the menu bar.
     */
    UInt16 MenuCount;

    /**
     * @brief Content hash for change detection.
     */
    UInt32 ContentHash;

    /**
     * @brief Total size of the wire data in bytes (including this header).
     */
    UInt32 TotalSize;
  };

  /**
   * @brief Request to register a menu provider with the context server.
   */
  struct ContextRegisterProviderRequest : public ContextRequestWithReply {
    /**
     * @brief The menu layer this provider contributes to.
     */
    Menus::MenuLayer Layer;

    /**
     * @brief Within-layer priority (higher value wins conflicts).
     */
    UInt16 Priority;

    /**
     * @brief The process ID of the registering application.
     */
    UInt32 ProcessID;
  };

  /**
   * @brief Result of a @ref ContextOperation::RegisterProvider request.
   */
  struct ContextRegisterProviderResult {
    /**
     * @brief `true` if registration succeeded.
     */
    bool Success;

    /**
     * @brief The provider ID assigned by the context server. Use this ID
     *        in subsequent requests.
     */
    Menus::ProviderID ProviderID;
  };

  /**
   * @brief Request to unregister a menu provider.
   */
  struct ContextUnregisterProviderRequest : public ContextRequest {
    /**
     * @brief The provider ID to unregister.
     */
    Menus::ProviderID ProviderID;
  };

  /**
   * @brief Request to push menu contributions from a provider. The
   *        actual contribution data is in a shared buffer identified by
   *        @ref BufferID.
   */
  struct ContextSetContributionsRequest : public ContextRequestWithReply {
    /**
     * @brief The provider ID (returned by RegisterProvider).
     */
    Menus::ProviderID ProviderID;

    /**
     * @brief Shared buffer containing the serialized contribution
     *        (wire format: @ref WireContributionHeader + data).
     */
    SharedBufferID BufferID;
  };

  /**
   * @brief Acknowledgment sent after the server has read the contribution
   *        buffer. The client may detach the shared buffer after receiving
   *        this.
   */
  struct ContextSetContributionsAck {
    /**
     * @brief `true` if the contributions were accepted.
     */
    bool Success;
  };

  /**
   * @brief Request to update the system focus context.
   *
   * Sent by the application server when window focus changes, or by
   * applications when their internal selection state changes.
   */
  struct ContextUpdateFocusContextRequest : public ContextRequest {
    /**
     * @brief The new focus context.
     */
    Menus::FocusContext Context;
  };

  /**
   * @brief Request to invoke a menu action.
   */
  struct ContextInvokeActionRequest : public ContextRequestWithReply {
    /**
     * @brief The action ID to invoke.
     */
    Menus::ActionID Action;

    /**
     * @brief The focus context at the time of invocation.
     */
    Menus::FocusContext Context;
  };

  /**
   * @brief Result of a @ref ContextOperation::InvokeAction request.
   */
  struct ContextInvokeActionResult {
    /**
     * @brief `true` if the action was handled by a provider.
     */
    bool Handled;
  };

  /**
   * @brief Request to retrieve the current composed menu bar state.
   */
  struct ContextGetMenuBarStateRequest : public ContextRequestWithReply {};

  /**
   * @brief Result of a @ref ContextOperation::GetMenuBarState request.
   *        The menu bar data is in a shared buffer.
   */
  struct ContextGetMenuBarStateResult {
    /**
     * @brief `true` if the state was retrieved successfully.
     */
    bool Success;

    /**
     * @brief Shared buffer containing the serialized menu bar state
     *        (wire format: @ref WireMenuBarHeader + data). The caller
     *        must attach, read, and detach this buffer.
     */
    SharedBufferID BufferID;
  };
}
