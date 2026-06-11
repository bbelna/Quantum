/**
 * @file Servers/App/Cursor/CursorManager.cpp
 * @brief Implements @ref @QAppSrv::CursorManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Clients/FileSystemClient.hpp>
#include <Quantum/Cursors/QCUR.hpp>
#include <Quantum/Theme.hpp>

#include <AppServerTypes.hpp>

#include "CursorManager.hpp"

namespace Quantum::Servers::App::Cursor {
  CursorManager::CursorManager(
    GraphicsClient& graphics,
    UInt16 screenWidth,
    UInt16 screenHeight
  ) :
    _graphicsClient(graphics),
    _screenWidth(screenWidth),
    _screenHeight(screenHeight)
  {
    constexpr Size bitmapSize
      = static_cast<Size>(GraphicsClient::CursorWidth)
      * GraphicsClient::CursorHeight
      * sizeof(UInt32);

    _expandedBitmap = reinterpret_cast<UInt32*>(AllocateBlock(bitmapSize));
  }

  CursorManager::~CursorManager() {
    if (_expandedBitmap) {
      FreeBlock(reinterpret_cast<UIntPtr>(_expandedBitmap));
    }

    if (_cursorFileBuffer != 0) {
      FreeBlock(_cursorFileBuffer);
    }
  }

  void CursorManager::Init() {
    // center the cursor on the screen
    _position = Point(
      static_cast<Int16>(_screenWidth / 2),
      static_cast<Int16>(_screenHeight / 2)
    );

    _loadCursors();

    // register the arrow cursor with the graphics server; fall back to
    // the hardcoded bitmap if the QCUR file was not available
    const UInt32* arrowPixels = _cursorSet.GetCursor(CursorType::Arrow);

    if (arrowPixels) {
      _sendBitmapWithShadow(arrowPixels);
    }
  }

  Point CursorManager::UpdatePosition(Int16 dx, Int16 dy) {
    Int16 newX = static_cast<Int16>(_position.X + dx);
    Int16 newY = static_cast<Int16>(_position.Y + dy);

    // clamp to screen bounds
    if (newX < 0) {
      newX = 0;
    } else if (newX >= static_cast<Int16>(_screenWidth)) {
      newX = static_cast<Int16>(_screenWidth - 1);
    }

    if (newY < 0) {
      newY = 0;
    } else if (newY >= static_cast<Int16>(_screenHeight)) {
      newY = static_cast<Int16>(_screenHeight - 1);
    }

    _position = Point(newX, newY);

    _graphicsClient.MoveCursor(_position);

    return _position;
  }

  void CursorManager::MoveTo(Point p, bool suppressFlush) {
    _position = p;

    _graphicsClient.MoveCursor(
      _position,
      suppressFlush
    );
  }

  Point CursorManager::GetPosition() const {
    return _position;
  }

  void CursorManager::SetCursor(CursorType type) {
    if (type == _currentCursor) return;

    _currentCursor = type;

    const UInt32* pixels = _cursorSet.GetCursor(type);

    if (pixels) {
      _sendBitmapWithShadow(pixels);
    }
  }

  void CursorManager::SetResizeCursor(bool resize) {
    SetCursor(
      resize
        ? CursorType::ResizeNWSE
        : CursorType::Arrow
    );
  }

  void CursorManager::Show(bool visible) {
    _graphicsClient.ShowCursor(visible);
  }

  void CursorManager::_sendBitmapWithShadow(const UInt32* pixels) {
    constexpr UInt8 artWidth = GraphicsClient::CursorArtWidth;
    constexpr UInt8 artHeight = GraphicsClient::CursorArtHeight;
    constexpr UInt8 outWidth = GraphicsClient::CursorWidth;
    constexpr UInt8 outHeight = GraphicsClient::CursorHeight;
    constexpr UInt8 shadowSize = Theme::CursorShadowSize;
    constexpr UInt8 shadowAlpha = Theme::CursorShadowAlpha;
    constexpr Int8 offsetX = Theme::CursorShadowOffsetX;
    constexpr Int8 offsetY = Theme::CursorShadowOffsetY;

    if (_expandedBitmap) {
      // clear to transparent
      for (
        UInt16 i = 0;
        i < static_cast<UInt16>(outWidth) * outHeight;
        ++i
      ) {
        _expandedBitmap[i] = GraphicsClient::CursorTransparent;
      }

      // for each non-transparent cursor pixel, render shadow layers around it
      // using concentric expansion with quadratic alpha falloff (matches
      // Canvas::RenderShadow)
      if (Theme::EnableCursorShadow) {
        for (
          UInt8 artRow = 0;
          artRow < artHeight;
          ++artRow
        ) {
          for (
            UInt8 artColumn = 0;
            artColumn < artWidth;
            ++artColumn
          ) {
            UInt32 pixel = pixels[
              static_cast<UInt32>(artRow) * artWidth
              + artColumn
            ];

            if (pixel != GraphicsClient::CursorTransparent) {
              // stamp shadow pixels around this opaque cursor pixel
              for (
                Int8 layer = static_cast<Int8>(shadowSize);
                layer >= 1;
                --layer
              ) {
                UInt32 progress = static_cast<UInt32>(shadowSize + 1 - layer);
                UInt8 alpha = static_cast<UInt8>(
                    shadowAlpha * progress * progress
                  / (static_cast<UInt32>(shadowSize) * shadowSize)
                );

                if (alpha != 0) {
                  for (
                    Int8 dy = static_cast<Int8>(-layer);
                    dy <= layer;
                    ++dy
                  ) {
                    for (
                      Int8 dx = static_cast<Int8>(-layer);
                      dx <= layer;
                      ++dx
                    ) {
                      Int16 outColumn = static_cast<Int16>(
                        artColumn + offsetX + dx
                      );
                      Int16 outRow = static_cast<Int16>(
                        artRow + offsetY + dy
                      );

                      if (
                        outColumn < 0 ||
                        outColumn >= outWidth
                      ) {
                        Math::Clamp(
                          outColumn,
                          static_cast<Int16>(0),
                          static_cast<Int16>(outWidth - 1)
                        );
                      }

                      if (
                        outRow < 0 ||
                        outRow >= outHeight
                      ) {
                        Math::Clamp(
                          outRow,
                          static_cast<Int16>(0),
                          static_cast<Int16>(outHeight - 1)
                        );
                      }

                      UInt32& destination = _expandedBitmap[
                        outRow * outWidth
                        + outColumn
                      ];

                      // inner layers overwrite outer layers (higher alpha
                      // closer to the shape); cursor artwork stamps on top
                      // in the second pass
                      UInt8 existing = static_cast<UInt8>(
                        (destination >> 24) & 0xFF
                      );

                      if (alpha > existing) {
                        destination = static_cast<UInt32>(alpha) << 24;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }

      // stamp the actual cursor artwork on top (overwrites any shadow pixel)
      for (
        UInt8 artRow = 0;
        artRow < artHeight;
        ++artRow
      ) {
        for (
          UInt8 artColumn = 0;
          artColumn < artWidth;
          ++artColumn
        ) {
          UInt32 pixel = pixels[
            static_cast<UInt32>(artRow) * artWidth
            + artColumn
          ];

          if (pixel != GraphicsClient::CursorTransparent) {
            _expandedBitmap[artRow * outWidth + artColumn] = pixel;
          }
        }
      }

      _graphicsClient.SetCursorBitmap(_expandedBitmap);
    }
  }

  void CursorManager::_loadCursors() {
    namespace FSABI = ::Quantum::Servers::FileSystem::ABI;
    Quantum::Clients::FileSystemClient fileSystem;

    const char* path = "QUANTUM/System/Cursors/System.qcur";

    FSABI::FileSystemFileStat stat = {};

    if (!fileSystem.Stat(path, &stat) || stat.Size == 0) return;

    FSABI::FileHandle handle = fileSystem.Open(
      path,
      static_cast<UInt32>(FSABI::FileSystemOpenFlags::Read)
    );

    if (handle == 0) return;

    _cursorFileBuffer = AllocateBlock(stat.Size);

    if (_cursorFileBuffer == 0) {
      fileSystem.Close(handle);

      return;
    }

    auto* data = reinterpret_cast<UInt8*>(_cursorFileBuffer);

    Int32 bytesRead = fileSystem.Read(handle, data, stat.Size, 0);

    fileSystem.Close(handle);

    if (bytesRead <= 0) {
      FreeBlock(_cursorFileBuffer);
      _cursorFileBuffer = 0;

      return;
    }

    _cursorSet = QCURParser::Parse(
      data, static_cast<Size>(bytesRead)
    );

    if (!_cursorSet.PixelData) {
      FreeBlock(_cursorFileBuffer);
      _cursorFileBuffer = 0;
    }
  }
}
