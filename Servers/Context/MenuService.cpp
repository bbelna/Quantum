/**
 * @file Servers/Context/MenuService.cpp
 * @brief Implements @ref @QCtxSrv::MenuService.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "MenuService.hpp"

namespace Quantum::Servers::Context {
  void RegisteredProvider::FreeContribution() {
    if (Contribution) {
      Contribution->FreeAll();

      delete Contribution;

      Contribution = nullptr;
    }

    delete[] SuppressedActions;

    SuppressedActions = nullptr;
    SuppressedCount = 0;
    HasContribution = false;
  }

  MenuService::~MenuService() {
    for (Size index = 0; index < MaxProviders; ++index) {
      _providers[index].FreeContribution();
    }

    if (_currentBar) {
      _currentBar->FreeAll();

      delete _currentBar;
    }
  }

  ProviderID MenuService::RegisterProvider(
    UInt32 processID,
    MenuLayer layer,
    UInt16 priority
  ) {
    for (Size index = 0; index < MaxProviders; ++index) {
      if (_providers[index].ProviderID != 0) continue;

      ProviderID assignedID = _nextProviderID++;

      _providers[index].ProviderID = assignedID;
      _providers[index].ProcessID = processID;
      _providers[index].Layer = layer;
      _providers[index].Priority = priority;
      _providers[index].HasContribution = false;
      _providers[index].Contribution = nullptr;
      _providers[index].SuppressedActions = nullptr;
      _providers[index].SuppressedCount = 0;

      _log.Write(
        LogLevel::Trace,
        "Registered provider %u for PID %u (layer %u)",
        assignedID,
        processID,
        layer
      );

      return assignedID;
    }

    _log.Write(
      LogLevel::Warning,
      "Provider table full, registration rejected"
    );

    return 0;
  }

  void MenuService::UnregisterProvider(ProviderID providerID) {
    for (Size index = 0; index < MaxProviders; ++index) {
      if (_providers[index].ProviderID != providerID) continue;

      _log.Write(
        LogLevel::Trace,
        "Unregistered provider %u",
        providerID
      );

      _providers[index].FreeContribution();

      _providers[index].ProviderID = 0;
      _providers[index].ProcessID = 0;

      _rebuildMenuBar();

      return;
    }
  }

  void MenuService::SetContributionsFromBuffer(
    ProviderID providerID,
    SharedBufferID bufferID
  ) {
    RegisteredProvider* provider = nullptr;

    for (Size index = 0; index < MaxProviders; ++index) {
      if (_providers[index].ProviderID == providerID) {
        provider = &_providers[index];

        break;
      }
    }

    if (!provider) {
      _log.Write(
        LogLevel::Warning,
        "Cannot set contributions for unknown provider %u",
        providerID
      );

      return;
    }

    // attach the shared buffer from the client
    UIntPtr bufferAddress = _kernel.AttachSharedBuffer(bufferID);

    if (bufferAddress == 0) {
      _log.Write(
        LogLevel::Warning,
        "Failed to attach contribution buffer %u",
        bufferID
      );

      return;
    }

    auto* wireHeader = reinterpret_cast<const CtxABI::WireContributionHeader*>(
      bufferAddress
    );

    // free any previous contribution
    provider->FreeContribution();

    // deserialize suppressed actions
    const UInt8* cursor
      = reinterpret_cast<const UInt8*>(bufferAddress)
        + sizeof(CtxABI::WireContributionHeader);

    if (wireHeader->SuppressedCount > 0) {
      provider->SuppressedActions = new ActionID[wireHeader->SuppressedCount];
      provider->SuppressedCount = wireHeader->SuppressedCount;

      for (Size index = 0; index < wireHeader->SuppressedCount; ++index) {
        auto* action = reinterpret_cast<const ActionID*>(cursor);

        provider->SuppressedActions[index] = *action;

        cursor += sizeof(ActionID);
      }
    }

    // deserialize menus into a MenuBarState
    provider->Contribution = new MenuBarState{};

    for (UInt16 menuIndex = 0; menuIndex < wireHeader->MenuCount; ++menuIndex) {
      auto* menuHeader
        = reinterpret_cast<const CtxABI::WireMenuHeader*>(cursor);

      cursor += sizeof(CtxABI::WireMenuHeader);

      Menu* menu = provider->Contribution->FindOrCreateMenu(
        menuHeader->ID,
        menuHeader->Position,
        menuHeader->Title
      );

      if (!menu) break;

      menu->IsFocusMenu = menuHeader->IsFocusMenu;

      for (UInt16 itemIndex = 0; itemIndex < menuHeader->ItemCount; ++itemIndex) {
        auto* item = reinterpret_cast<const MenuItem*>(cursor);

        menu->AddItem(*item);

        cursor += sizeof(MenuItem);
      }
    }

    provider->HasContribution = true;

    _kernel.DetachSharedBuffer(bufferAddress);

    _log.Write(
      LogLevel::Trace,
      "Provider %u pushed %u menu(s)",
      providerID,
      provider->Contribution->MenuCount
    );

    _rebuildMenuBar();
  }

  void MenuService::AddSystemMenu(Menu&& menu) {
    if (_systemMenuCount >= _systemMenuCapacity) {
      Size newCapacity = (_systemMenuCapacity == 0) ? 4 : _systemMenuCapacity * 2;
      auto* newMenus = new Menu[newCapacity];

      for (Size index = 0; index < _systemMenuCount; ++index) {
        newMenus[index] = _systemMenus[index];

        _systemMenus[index].Items = nullptr;
        _systemMenus[index].ItemCount = 0;
        _systemMenus[index].ItemCapacity = 0;
      }

      delete[] _systemMenus;

      _systemMenus = newMenus;
      _systemMenuCapacity = newCapacity;
    }

    _systemMenus[_systemMenuCount] = menu;

    // take ownership, clear source
    menu.Items = nullptr;
    menu.ItemCount = 0;
    menu.ItemCapacity = 0;

    _systemMenuCount++;

    _rebuildMenuBar();
  }

  void MenuService::UpdateFocusContext(const FocusContext& context) {
    _currentContext = context;

    _rebuildMenuBar();
  }

  const ActionProviderMapping* MenuService::FindProviderForAction(
    ActionID action
  ) const {
    for (Size index = 0; index < _actionMapCount; ++index) {
      if (_actionMap[index].Action == action) {
        return &_actionMap[index];
      }
    }

    return nullptr;
  }

  SharedBufferID MenuService::SerializeMenuBarState() const {
    if (!_currentBar) return 0;

    // compute size
    UInt32 size = sizeof(CtxABI::WireMenuBarHeader);

    for (Size menuIndex = 0; menuIndex < _currentBar->MenuCount; ++menuIndex) {
      size += sizeof(CtxABI::WireMenuHeader);
      size += _currentBar->Menus[menuIndex].ItemCount * sizeof(MenuItem);
    }

    SharedBufferID bufferID
      = _kernel.CreateSharedBuffer(size);

    if (bufferID == 0) return 0;

    UIntPtr bufferAddress = _kernel.AttachSharedBuffer(bufferID);

    if (bufferAddress == 0) return 0;

    auto* header = reinterpret_cast<CtxABI::WireMenuBarHeader*>(
      bufferAddress
    );

    header->MenuCount = static_cast<UInt16>(_currentBar->MenuCount);
    header->ContentHash = _currentBar->ContentHash;
    header->TotalSize = size;

    UInt8* cursor
      = reinterpret_cast<UInt8*>(bufferAddress)
        + sizeof(CtxABI::WireMenuBarHeader);

    for (Size menuIndex = 0; menuIndex < _currentBar->MenuCount; ++menuIndex) {
      const Menu& menu = _currentBar->Menus[menuIndex];

      auto* menuHeader = reinterpret_cast<CtxABI::WireMenuHeader*>(cursor);

      menuHeader->ID = menu.ID;

      CString::Copy(menu.Title, menuHeader->Title, MaxMenuTitleLength);

      menuHeader->Position = menu.Position;
      menuHeader->ItemCount = static_cast<UInt16>(menu.ItemCount);
      menuHeader->IsFocusMenu = menu.IsFocusMenu;

      cursor += sizeof(CtxABI::WireMenuHeader);

      for (Size itemIndex = 0; itemIndex < menu.ItemCount; ++itemIndex) {
        auto* item = reinterpret_cast<MenuItem*>(cursor);

        *item = menu.Items[itemIndex];

        cursor += sizeof(MenuItem);
      }
    }

    // detach the previous serialized buffer now, the client has had
    // enough time to attach since the last call
    const_cast<MenuService*>(this)->_detachPreviousSerializedBuffer();

    // keep this buffer alive until the next serialization so the client
    // has time to attach it
    const_cast<MenuService*>(this)->_lastSerializedBuffer = bufferAddress;

    return bufferID;
  }

  void MenuService::_detachPreviousSerializedBuffer() {
    if (_lastSerializedBuffer != 0) {
      _kernel.DetachSharedBuffer(_lastSerializedBuffer);

      _lastSerializedBuffer = 0;
    }
  }

  void MenuService::_rebuildMenuBar() {
    // free the old bar
    if (_currentBar) {
      _currentBar->FreeAll();

      delete _currentBar;
    }

    _currentBar = new MenuBarState{};
    _actionMapCount = 0;

    // add system menus first (always present)
    for (Size index = 0; index < _systemMenuCount; ++index) {
      const Menu& systemMenu = _systemMenus[index];

      Menu* targetMenu = _currentBar->FindOrCreateMenu(
        systemMenu.ID,
        systemMenu.Position,
        systemMenu.Title
      );

      if (!targetMenu) continue;

      targetMenu->IsFocusMenu = systemMenu.IsFocusMenu;

      for (Size itemIndex = 0; itemIndex < systemMenu.ItemCount; ++itemIndex) {
        targetMenu->AddItem(systemMenu.Items[itemIndex]);

        if (
          systemMenu.Items[itemIndex].Action != 0 &&
          _actionMapCount < MaxTrackedActions
        ) {
          _actionMap[_actionMapCount].Action
            = systemMenu.Items[itemIndex].Action;
          _actionMap[_actionMapCount].Provider = 0;
          _actionMap[_actionMapCount].ProcessID = 0;
          _actionMapCount++;
        }
      }
    }

    // merge contributions in layer order (lowest first, highest last so
    // higher layers override)
    for (
      UInt8 layer = 0;
      layer <= static_cast<UInt8>(MenuLayer::Object);
      ++layer
    ) {
      for (Size index = 0; index < MaxProviders; ++index) {
        RegisteredProvider& provider = _providers[index];

        if (provider.ProviderID == 0) continue;
        if (!provider.HasContribution) continue;
        if (static_cast<UInt8>(provider.Layer) != layer) continue;

        _mergeContribution(_currentBar, provider);
      }
    }

    _currentBar->SortByPosition();
    _currentBar->ContentHash = _computeContentHash();
  }

  void MenuService::_mergeContribution(
    MenuBarState* bar,
    const RegisteredProvider& provider
  ) {
    // apply suppressions
    for (Size index = 0; index < provider.SuppressedCount; ++index) {
      ActionID suppressedAction = provider.SuppressedActions[index];

      for (Size menuIndex = 0; menuIndex < bar->MenuCount; ++menuIndex) {
        Menu& menu = bar->Menus[menuIndex];

        for (Size itemIndex = 0; itemIndex < menu.ItemCount; ++itemIndex) {
          if (menu.Items[itemIndex].Action == suppressedAction) {
            menu.Items[itemIndex].State = ActionState::Hidden;
          }
        }
      }
    }

    // merge menus from the contribution
    if (!provider.Contribution) return;

    for (
      Size menuIndex = 0;
      menuIndex < provider.Contribution->MenuCount;
      ++menuIndex
    ) {
      const Menu& sourceMenu = provider.Contribution->Menus[menuIndex];

      if (sourceMenu.ItemCount == 0) continue;

      // focus menus only show when their provider's process owns
      // the focused window
      if (sourceMenu.IsFocusMenu) {
        if (_currentContext.WindowID == 0) continue;
        if (provider.ProcessID != _currentContext.ProcessID) continue;
      }

      Menu* targetMenu = bar->FindOrCreateMenu(
        sourceMenu.ID,
        sourceMenu.Position,
        sourceMenu.Title
      );

      if (!targetMenu) continue;

      if (sourceMenu.IsFocusMenu) {
        targetMenu->IsFocusMenu = true;
      }

      for (Size itemIndex = 0; itemIndex < sourceMenu.ItemCount; ++itemIndex) {
        const MenuItem& sourceItem = sourceMenu.Items[itemIndex];

        targetMenu->AddItem(sourceItem);

        // track which provider contributed this action
        if (
          sourceItem.Action != 0 &&
          _actionMapCount < MaxTrackedActions
        ) {
          _actionMap[_actionMapCount].Action = sourceItem.Action;
          _actionMap[_actionMapCount].Provider = provider.ProviderID;
          _actionMap[_actionMapCount].ProcessID = provider.ProcessID;
          _actionMapCount++;
        }
      }
    }
  }

  UInt32 MenuService::_computeContentHash() const {
    if (!_currentBar) return 0;

    UInt32 hash = 0x811C9DC5;

    for (Size menuIndex = 0; menuIndex < _currentBar->MenuCount; ++menuIndex) {
      const Menu& menu = _currentBar->Menus[menuIndex];

      hash ^= menu.Position;
      hash *= 0x01000193;

      for (Size itemIndex = 0; itemIndex < menu.ItemCount; ++itemIndex) {
        hash ^= menu.Items[itemIndex].Action;
        hash *= 0x01000193;
        hash ^= static_cast<UInt32>(menu.Items[itemIndex].State);
        hash *= 0x01000193;
      }
    }

    return hash;
  }
}
