/**
 * @file Apps/Terminal/TerminalApp.hpp
 * @brief Implements @ref @QApps::Terminal::Terminal.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Fonts/PSF.hpp>

#include "Terminal.hpp"

namespace Quantum::Apps::Terminal {
  static constexpr UInt32 TerminalBackground = Theme::WindowBackground;
  static constexpr UInt32 TerminalForeground = Theme::TextForeground;

  Terminal::Terminal() :
    _window(
      100,
      80,
      640,
      480,
      "Terminal",
      TerminalBackground,
      false,
      true,
      true,
      true,
      TerminalBackground
    ),
    _console(_window.GetCanvas())
  {
    AddWindow(_window);
  }

  Terminal::~Terminal() {
    if (_dialog) {
      _dialog->Dismiss();

      delete _dialog;
    } else {
      if (_inStream.IsValid()) {
        _inStream.Close();
      }

      _outStream.Close();
      _unregisterMenus();

      if (_fontData != 0) {
        FreeBlock(_fontData);
      }
    }
  }

  void Terminal::Init() {
    _console.SetBackgroundColor(TerminalBackground);
    _console.SetForegroundColor(TerminalForeground);
    _console.SetInvalidateCallback(
      [](
        void* context,
        Rectangle dirtyRectangle,
        Int16 shiftDeltaY
      ) {
        Window* window = static_cast<Window*>(context);

        if (shiftDeltaY != 0) {
          window->InvalidateWithShift(
            shiftDeltaY,
            dirtyRectangle
          );
        } else {
          window->Invalidate(dirtyRectangle);
        }
      },
      &_window
    );
    _console.SetPadding(6);

    _loadFont();

    _window.SetMinimumSize(
      120,
      80
    );
    _window.Add(_console);

    if (!_spawnChild()) {
      _console.Write(":( Failed to launch qsh.qx\n");
      _console.Flush();
    }
  }

  void Terminal::_loadFont() {
    FileSystemVolumeInfo volumes[1];

    if (
      _fileSystem.ListVolumes(
        volumes,
        1
      ) > 0
    ) {
      char fontPath[FileSystemClient::MaxPathLength];

      CString::Format(
        fontPath,
        sizeof(fontPath),
        "%s/System/Fonts/Terminus12.psf",
        volumes[0].Label
      );

      FileSystemFileStat fileStat = {};

      if (
        _fileSystem.Stat(fontPath, &fileStat) &&
        fileStat.Size > 0
      ) {
        FileHandle fileHandle = _fileSystem.Open(
          fontPath,
          static_cast<UInt32>(FileSystemOpenFlags::Read)
        );

        if (fileHandle > 0) {
          _fontData = new UInt8[fileStat.Size];

          if (_fontData) {
            Int32 bytesRead = _fileSystem.Read(
              fileHandle,
              reinterpret_cast<void*>(_fontData),
              fileStat.Size,
              0
            );

            if (bytesRead > 0) {
              _font = PSFParser::Parse(
                reinterpret_cast<const UInt8*>(_fontData),
                static_cast<Size>(bytesRead)
              );

              if (_font.GlyphData) {
                _window
                  .GetCanvas()
                  .GetPainter()
                  .SetFont(_font);
              } else {
                delete[] _fontData;

                _fontData = 0;
              }
            } else {
              delete[] _fontData;

              _fontData = 0;
            }
          }
        }

        _fileSystem.Close(fileHandle);
      }
    }
  }

  bool Terminal::_spawnChild() {
    ProcessID childProcessID = InvalidProcessID;
    FileSystemVolumeInfo volumes[1];
    SharedBufferID stdinBufferID = 0;
    SharedBufferID stdoutBufferID = 0;
    FileSystemFileStat fileStat = {};
    FileHandle fileHandle = 0;
    UInt8* fileData = 0;

    // detect the boot volume
    if (
      _fileSystem.ListVolumes(
        volumes,
        1
      ) > 0
    ) {
      // create two streams: one for stdin, one for stdout
      if (
        _streams.CreateStream(stdinBufferID) &&
        _streams.CreateStream(stdoutBufferID)
      ) {
        // open the stdin stream for writing (parent -> child)
        _inStream = Stream::Open(stdinBufferID);

        // attach to the stdout stream for reading (child -> parent)
        _outStream.AttachToBuffer(stdoutBufferID);

        if (
          _inStream.IsValid() &&
          _outStream.IsValid()
        ) {
          // load the qsh binary from the boot volume
          char shellPath[FileSystemClient::MaxPathLength];

          CString::Format(
            shellPath,
            sizeof(shellPath),
            "%s/Binaries/qsh.qx",
            volumes[0].Label
          );

          fileHandle = _fileSystem.Open(
            shellPath,
            static_cast<UInt32>(FileSystemOpenFlags::Read)
          );

          if (
            _fileSystem.Stat(
              shellPath,
              &fileStat
            ) &&
            fileStat.Type == FileSystemEntryType::Regular &&
            fileStat.Size > 0 &&
            fileHandle > 0
          ) {
            fileData = new UInt8[fileStat.Size];

            if (fileData) {
              Int32 bytesRead = _fileSystem.Read(
                fileHandle,
                reinterpret_cast<void*>(fileData),
                fileStat.Size,
                0
              );

              _fileSystem.Close(fileHandle);

              // prepare inherited streams: stdin, stdout, stderr
              SharedBufferID streamBufferIDs[3] = {};

              streamBufferIDs[0] = stdinBufferID;
              streamBufferIDs[1] = stdoutBufferID;
              streamBufferIDs[2] = stdoutBufferID;

              const char* arguments[] = { "qsh.qx" };
              char currentWorkingDirectory[FileSystemClient::MaxPathLength];

              CString::Format(
                currentWorkingDirectory,
                sizeof(currentWorkingDirectory),
                "%s",
                volumes[0].Label
              );

              childProcessID = _run.LoadELF(
                shellPath,
                currentWorkingDirectory,
                reinterpret_cast<const void*>(fileData),
                static_cast<Size>(bytesRead),
                1,
                arguments,
                3,
                streamBufferIDs
              );
            }
          }
        }
      }
    }

    if (stdinBufferID > 0) {
      _streams.CloseReader(stdinBufferID);
    }

    if (stdoutBufferID > 0) {
      _streams.CloseReader(stdoutBufferID);
    }

    if (fileData) {
      delete[] fileData;
    }

    return childProcessID != InvalidProcessID;
  }

  int Terminal::Run() {
    Init();

    _window.DrawAll();
    _window.Invalidate();

    OnReady();

    while (IsRunning()) {
      // drain all pending window events (non-blocking)
      WindowEvent event;

      while (_window.TryGetWindowEvent(&event)) {
        // intercept keyboard events and forward to child stdin
        if (event.Type == WindowEventType::Keyboard) {
          if (
            !_childExited &&
            event.KeyboardEvent.Type == InputEventType::KeyDown
          ) {
            char character = event.KeyboardEvent.Character;
            UInt8 scancode = event.KeyboardEvent.Scancode;

            // normalize backspace: scancode 0x0E with no character
            if (
              (
                scancode == 0x0E ||
                character == '\b'
              ) &&
              character == 0
            ) {
              character = '\b';
            }

            if (
              character != 0 &&
              _inStream.IsValid()
            ) {
              _inStream.Write(&character, 1);
            }
          }

          continue;
        } else if (_window.DispatchEvent(event)) {
          // dispatch non-keyboard events normally

          continue;
        } else {
          switch (event.Type) {
            case WindowEventType::Close: {
              // close stdin to signal the child to exit
              if (!_childExited) {
                _inStream.Close();
              }

              RequestExit();

              break;
            }

            case WindowEventType::Resize: {
              OnResize(
                _window,
                event.ContentWidth,
                event.ContentHeight
              );

              break;
            }

            case WindowEventType::MouseScroll: {
              OnScroll(
                _window,
                event.ScrollDelta
              );

              break;
            }

            case WindowEventType::Deactivated: {
              _window.ClearFocus();

              break;
            }

            case WindowEventType::MenuAction: {
              OnMenuAction(
                _window,
                event.MenuActionID
              );

              break;
            }

            default: {
              break;
            }
          }
        }
      }

      // pump non-blocking dialog events
      if (_dialog) {
        _dialog->TryPumpEvent();

        if (!_dialog->IsOpen()) {
          _dialog->Dismiss();

          delete _dialog;

          _dialog = nullptr;
        }
      }

      // poll child stdout and write to terminal
      if (!_childExited) {
        bool hasNewOutput = false;
        char readBuffer[256];
        Size bytesRead = _outStream.Read(
          readBuffer,
          sizeof(readBuffer) - 1
        );

        if (bytesRead > 0) {
          readBuffer[bytesRead] = '\0';

          _console.Write(readBuffer);

          hasNewOutput = true;
        }

        if (_outStream.IsClosed()) {
          // drain remaining data
          for (;;) {
            bytesRead = _outStream.Read(
              readBuffer,
              sizeof(readBuffer) - 1
            );

            if (bytesRead == 0) {
              break;
            } else {
              readBuffer[bytesRead] = '\0';

              _console.Write(readBuffer);

              hasNewOutput = true;
            }
          }

          _childExited = true;
        }

        if (hasNewOutput) {
          _console.Flush();
        }
      }

      Thread::Yield();
    }

    return 0;
  }

  void Terminal::OnReady() {
    // notify the context server that this window is focused, ensuring
    // the focus context is set before we register menus
    FocusContext context = {};

    context.WindowID = _window.GetResourceID();
    context.ProcessID = Process::GetCurrentProcessID();

    _context.UpdateFocusContext(context);

    _registerMenus();
  }

  void Terminal::OnResize(
    Window&,
    UInt16 width,
    UInt16 height
  ) {
    _console.Resize(
      width,
      height
    );
  }

  void Terminal::OnScroll(
    Window&,
    Int8 delta
  ) {
    _console.ScrollBy(delta);
  }

  void Terminal::OnMenuAction(
    Window&,
    UInt32 actionID
  ) {
    // action 0x1000 = About
    if (
      actionID == 0x1000 &&
      !_dialog
    ) {
      DialogButton buttons[] = {
        DialogButton(
          "Cool",
          [](Dialog& dialog) {
            dialog.Close();
          }
        )
      };

      _dialog = new Dialog(
        _window,
        "About Terminal",
        "Terminal",
        "A simple command-line interface for\n"
        "interacting with the OS.\n\n"
        ""
        "Copyright (c) 2025-2026\n"
        "The Quantum Software Project",
        buttons,
        1
      );

      _dialog->ShowNonBlocking();
    }
  }

  void Terminal::_registerMenus() {
    ProviderID providerID = _context.RegisterProvider(
      MenuLayer::View,
      100
    );

    if (providerID > 0) {
      MenuBuilder builder;

      // create the Terminal focus menu (bold, leftmost)
      UInt8 terminalMenu = builder.CreateFocusMenu("Terminal");
      MenuItem aboutItem = {};

      aboutItem.Action = static_cast<ActionID>(WellKnownAction::AppDefined);

      CString::Copy(
        "About",
        aboutItem.Label,
        MaxMenuItemLabelLength
      );

      aboutItem.State = ActionState::Enabled;
      aboutItem.OrderHint = 100;

      builder.AddItem(
        terminalMenu,
        aboutItem
      );

      _context.SetContributions(
        providerID,
        builder
      );

      _menuProviderID = providerID;
    }
  }

  void Terminal::_unregisterMenus() {
    if (_menuProviderID > 0) {
      ContextClient client;

      client.UnregisterProvider(_menuProviderID);

      _menuProviderID = 0;
    }
  }
}
