/**
 * @file Clients/ContextClient.cpp
 * @brief Implements @ref @QClients::ContextClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>
#include "ContextClient.hpp"

namespace Quantum::Clients {
  ProviderID ContextClient::RegisterProvider(
    MenuLayer layer,
    UInt16 priority
  ) {
    ContextRegisterProviderRequest request = {};

    request.ABIVersion = ContextABIVersion;
    request.Operation = ContextOperation::RegisterProvider;
    request.Layer = layer;
    request.Priority = priority;
    request.ProcessID = Process::GetID();

    ContextRegisterProviderResult result = InvokeOS<
      ContextRegisterProviderResult,
      ContextRegisterProviderRequest
    >(ContextPortID, 0, request);

    if (!result.Success) return 0;

    return result.ProviderID;
  }

  void ContextClient::UnregisterProvider(ProviderID providerID) {
    ContextUnregisterProviderRequest request = {};

    request.ABIVersion = ContextABIVersion;
    request.Operation = ContextOperation::UnregisterProvider;
    request.ProviderID = providerID;

    SendOS(ContextPortID, request);
  }

  void ContextClient::SetContributions(
    ProviderID providerID,
    const MenuBuilder& builder
  ) {
    // serialize the builder into a shared buffer
    UInt32 bufferSize = _computeContributionSize(builder);
    SharedBufferID bufferID = CreateShared(bufferSize);

    if (bufferID == 0) return;

    UIntPtr bufferAddress = AttachShared(bufferID);

    if (bufferAddress == 0) return;

    _serializeContribution(
      builder,
      reinterpret_cast<UInt8*>(bufferAddress)
    );

    // send the request and wait for the server to acknowledge it read the
    // buffer before detaching
    ContextSetContributionsRequest request = {};

    request.ABIVersion = ContextABIVersion;
    request.Operation = ContextOperation::SetContributions;
    request.ProviderID = providerID;
    request.BufferID = bufferID;

    InvokeOS<
      ContextSetContributionsAck,
      ContextSetContributionsRequest
    >(ContextPortID, 0, request);

    // now safe to detach, the server has its own mapping
    DetachShared(bufferAddress);
  }

  void ContextClient::UpdateFocusContext(const FocusContext& context) {
    ContextUpdateFocusContextRequest request = {};

    request.ABIVersion = ContextABIVersion;
    request.Operation = ContextOperation::UpdateFocusContext;
    request.Context = context;

    SendOS(ContextPortID, request);
  }

  bool ContextClient::GetMenuBarState(MenuBarState* outState) {
    ContextGetMenuBarStateRequest request = {};

    request.ABIVersion = ContextABIVersion;
    request.Operation = ContextOperation::GetMenuBarState;

    ContextGetMenuBarStateResult result = InvokeOS<
      ContextGetMenuBarStateResult,
      ContextGetMenuBarStateRequest
    >(ContextPortID, 0, request);

    if (!result.Success || result.BufferID == 0) return false;

    // attach the shared buffer and deserialize
    UIntPtr bufferAddress = AttachShared(result.BufferID);

    if (bufferAddress == 0) return false;

    bool success = _deserializeMenuBarState(
      reinterpret_cast<const UInt8*>(bufferAddress),
      outState
    );

    DetachShared(bufferAddress);

    return success;
  }

  UInt32 ContextClient::_computeContributionSize(const MenuBuilder& builder) {
    UInt32 size = sizeof(WireContributionHeader);

    size += builder.GetSuppressedCount() * sizeof(ActionID);

    const Menu* menus = builder.GetMenus();

    for (Size index = 0; index < builder.GetMenuCount(); ++index) {
      size += sizeof(WireMenuHeader);
      size += menus[index].ItemCount * sizeof(MenuItem);
    }

    return size;
  }

  void ContextClient::_serializeContribution(
    const MenuBuilder& builder,
    UInt8* destination
  ) {
    WireContributionHeader* header = reinterpret_cast<WireContributionHeader*>(
      destination
    );

    header->MenuCount = static_cast<UInt16>(builder.GetMenuCount());
    header->SuppressedCount = static_cast<UInt16>(builder.GetSuppressedCount());

    UInt8* cursor = destination + sizeof(WireContributionHeader);

    // write suppressed actions
    const ActionID* suppressed = builder.GetSuppressedActions();

    for (Size index = 0; index < builder.GetSuppressedCount(); ++index) {
      ActionID* action = reinterpret_cast<ActionID*>(cursor);

      *action = suppressed[index];
      cursor += sizeof(ActionID);
    }

    // write menus
    const Menu* menus = builder.GetMenus();

    for (Size menuIndex = 0; menuIndex < builder.GetMenuCount(); ++menuIndex) {
      const Menu& menu = menus[menuIndex];
      WireMenuHeader* menuHeader = reinterpret_cast<WireMenuHeader*>(cursor);

      menuHeader->ID = menu.ID;

      CString::Copy(menu.Title, menuHeader->Title, MaxMenuTitleLength);

      menuHeader->Position = menu.Position;
      menuHeader->ItemCount = static_cast<UInt16>(menu.ItemCount);
      menuHeader->IsFocusMenu = menu.IsFocusMenu;

      cursor += sizeof(WireMenuHeader);

      for (Size itemIndex = 0; itemIndex < menu.ItemCount; ++itemIndex) {
        MenuItem* item = reinterpret_cast<MenuItem*>(cursor);

        *item = menu.Items[itemIndex];
        cursor += sizeof(MenuItem);
      }
    }

    header->TotalSize = static_cast<UInt32>(cursor - destination);
  }

  bool ContextClient::_deserializeMenuBarState(
    const UInt8* source,
    MenuBarState* outState
  ) {
    const WireMenuBarHeader* header
      = reinterpret_cast<const WireMenuBarHeader*>(source);
    const UInt8* cursor = source + sizeof(WireMenuBarHeader);

    outState->ContentHash = header->ContentHash;

    for (UInt16 menuIndex = 0; menuIndex < header->MenuCount; ++menuIndex) {
      const WireMenuHeader* menuHeader
        = reinterpret_cast<const WireMenuHeader*>(cursor);

      cursor += sizeof(WireMenuHeader);

      Menu* menu = outState->FindOrCreateMenu(
        menuHeader->ID,
        menuHeader->Position,
        menuHeader->Title
      );

      if (!menu) return false;

      menu->IsFocusMenu = menuHeader->IsFocusMenu;

      for (
        UInt16 itemIndex = 0;
        itemIndex < menuHeader->ItemCount;
        ++itemIndex
      ) {
        const MenuItem* item = reinterpret_cast<const MenuItem*>(cursor);

        menu->AddItem(*item);

        cursor += sizeof(MenuItem);
      }
    }

    return true;
  }
}
