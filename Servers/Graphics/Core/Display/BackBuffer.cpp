/**
 * @file Servers/Graphics/BackBuffer.cpp
 * @brief Implements @ref @QGfxSrv::BackBuffer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "BackBuffer.hpp"

namespace Quantum::Servers::Graphics {
  BackBuffer::BackBuffer(
    KernelClient& kernel,
    ServerLog& log
  ) :
    _kernel(kernel),
    _log(log)
  {
  }

  bool BackBuffer::Allocate(
    UInt16 width,
    UInt16 height,
    UInt8 bitsPerPixel
  ) {
    if (_buffer) {
      _kernel.DetachSharedBuffer(reinterpret_cast<UIntPtr>(_buffer));

      _buffer = nullptr;
      _bufferID = 0;
    }

    _format
      = bitsPerPixel <= 16
      ? PixelFormat::RGB565
      : PixelFormat::ARGB32;

    Size bufferSize
      = static_cast<Size>(width) * height * BytesPerPixel(_format);

    _bufferID = _kernel.CreateSharedBuffer(bufferSize);

    if (_bufferID == 0) {
      _log.Error(
        "Failed to create shared back buffer (%u bytes)",
        bufferSize
      );

      return false;
    }

    UIntPtr allocation = _kernel.AttachSharedBuffer(_bufferID);

    if (allocation == 0) {
      _log.Error(
        "Failed to attach shared back buffer (ID %u)",
        _bufferID
      );

      _bufferID = 0;

      return false;
    }

    _buffer = reinterpret_cast<void*>(allocation);
    _width = width;
    _height = height;

    Byte::Fill(
      _buffer,
      0,
      bufferSize
    );

    _log.Trace(
      "Shared back buffer allocated at 0x%x (%u bytes, ID %u, %ubpp)",
      allocation,
      bufferSize,
      _bufferID,
      static_cast<UInt32>(BitsPerPixel(_format))
    );

    return true;
  }

  void BackBuffer::FillRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 width,
    UInt16 height,
    UInt32 color
  ) {
    UInt16 right = x + width;
    UInt16 bottom = y + height;

    if (right > _width) {
      right = _width;
    }

    if (bottom > _height) {
      bottom = _height;
    }

    if (
      x < _width &&
      y < _height
    ) {
      UInt16 fillWidth = right - x;

      if (_format == PixelFormat::RGB565) {
        UInt16 nativeColor = Color::ToRGB565(color);
        UInt32 packed
          = static_cast<UInt32>(nativeColor)
          | (static_cast<UInt32>(nativeColor) << 16);
        UInt16* buffer = static_cast<UInt16*>(_buffer);

        for (
          UInt16 row = y;
          row < bottom;
          ++row
        ) {
          UInt16* destination = &buffer[
            static_cast<UInt32>(row) * _width
            + x
          ];
          UInt16 pairs = fillWidth >> 1;
          UInt32* destination32 = reinterpret_cast<UInt32*>(destination);

          for (
            UInt16 pair = 0;
            pair < pairs;
            ++pair
          ) {
            destination32[pair] = packed;
          }

          UInt16 column = static_cast<UInt16>(pairs << 1);

          if (column < fillWidth) {
            destination[column] = nativeColor;
          }
        }
      } else {
        UInt32* buffer = static_cast<UInt32*>(_buffer);

        for (
          UInt16 row = y;
          row < bottom;
          ++row
        ) {
          void* destination = &buffer[
            static_cast<UInt32>(row) * _width
            + x
          ];
          UInt32 count = fillWidth;

          #if defined(ARCH_IA32)
          asm volatile(
            "cld\n"
            "rep stosl"
            : "+D"(destination), "+c"(count)
            : "a"(color)
            : "memory"
          );
          #endif
        }
      }
    }
  }

  void BackBuffer::BlitBuffer(
    UInt16 x,
    UInt16 y,
    UInt16 width,
    UInt16 height,
    UInt32 transparent,
    const UInt32* pixels
  ) {
    if (_format == PixelFormat::RGB565) {
      UInt16* buffer = static_cast<UInt16*>(_buffer);

      for (
        UInt16 row = 0;
        row < height;
        ++row
      ) {
        UInt16 screenY = y + row;

        if (screenY < _height) {
          for (
            UInt16 column = 0;
            column < width;
            ++column
          ) {
            UInt16 screenX = x + column;

            if (screenX < _width) {
              UInt32 pixel = pixels[
                static_cast<UInt32>(row) * width
                + column
              ];

              if (pixel != transparent) {
                buffer[
                  static_cast<UInt32>(screenY) * _width
                  + screenX
                ] = Color::ToRGB565(pixel);
              }
            }
          }
        }
      }
    } else {
      UInt32* buffer = static_cast<UInt32*>(_buffer);

      for (
        UInt16 row = 0;
        row < height;
        ++row
      ) {
        UInt16 screenY = y + row;

        if (screenY < _height) {
          for (
            UInt16 column = 0;
            column < width;
            ++column
          ) {
            UInt16 screenX = x + column;

            if (screenX < _width) {
              UInt32 pixel = pixels[
                static_cast<UInt32>(row) * width
                + column
              ];

              if (pixel != transparent) {
                buffer[
                  static_cast<UInt32>(screenY) * _width
                  + screenX
                ] = pixel;
              }
            }
          }
        }
      }
    }
  }

  void BackBuffer::XORRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 width,
    UInt16 height,
    UInt32 color
  ) {
    UInt16 right = x + width;
    UInt16 bottom = y + height;

    if (right > _width) {
      right = _width;
    }

    if (bottom > _height) {
      bottom = _height;
    }

    if (
      x < _width &&
      y < _height
    ) {
      if (_format == PixelFormat::RGB565) {
        UInt16 nativeColor = Color::ToRGB565(color);
        UInt16* buffer = static_cast<UInt16*>(_buffer);

        for (
          UInt16 row = y;
          row < bottom;
          ++row
        ) {
          UInt32 rowOffset = static_cast<UInt32>(row) * _width;

          for (
            UInt16 column = x;
            column < right;
            ++column
          ) {
            buffer[rowOffset + column] ^= nativeColor;
          }
        }
      } else {
        UInt32* buffer = static_cast<UInt32*>(_buffer);

        for (
          UInt16 row = y;
          row < bottom;
          ++row
        ) {
          UInt32 rowOffset = static_cast<UInt32>(row) * _width;

          for (
            UInt16 column = x;
            column < right;
            ++column
          ) {
            buffer[rowOffset + column] ^= color;
          }
        }
      }
    }
  }

  void BackBuffer::ScreenBlit(
    UInt16 sourceX,
    UInt16 sourceY,
    UInt16 destinationX,
    UInt16 destinationY,
    UInt16 width,
    UInt16 height
  ) {
    if (_buffer && width > 0 && height > 0) {
      if (sourceX + width > _width) {
        width = _width - sourceX;
      }

      if (sourceY + height > _height) {
        height = _height - sourceY;
      }

      if (destinationX + width > _width) {
        width = _width - destinationX;
      }

      if (destinationY + height > _height) {
        height = _height - destinationY;
      }

      bool copyDown
        = destinationY > sourceY
       || (
          destinationY == sourceY &&
          destinationX > sourceX
        );

      if (_format == PixelFormat::RGB565) {
        UInt16* buffer = static_cast<UInt16*>(_buffer);

        if (copyDown) {
          for (
            Int16 row = static_cast<Int16>(height - 1);
            row >= 0;
            --row
          ) {
            UInt16* source = &buffer[
              static_cast<UInt32>(sourceY + row) * _width
              + sourceX
            ];
            UInt16* destination = &buffer[
              static_cast<UInt32>(destinationY + row) * _width
              + destinationX
            ];

            for (
              Int16 column = static_cast<Int16>(width - 1);
              column >= 0;
              --column
            ) {
              destination[column] = source[column];
            }
          }
        } else {
          for (
            UInt16 row = 0;
            row < height;
            ++row
          ) {
            UInt16* source = &buffer[
              static_cast<UInt32>(sourceY + row) * _width
              + sourceX
            ];
            UInt16* destination = &buffer[
              static_cast<UInt32>(destinationY + row) * _width
              + destinationX
            ];

            for (
              UInt16 column = 0;
              column < width;
              ++column
            ) {
              destination[column] = source[column];
            }
          }
        }
      } else {
        UInt32* buffer = static_cast<UInt32*>(_buffer);

        if (copyDown) {
          for (
            Int16 row = static_cast<Int16>(height - 1);
            row >= 0;
            --row
          ) {
            UInt32* source = &buffer[
              static_cast<UInt32>(sourceY + row) * _width
              + sourceX
            ];
            UInt32* destination = &buffer[
              static_cast<UInt32>(destinationY + row) * _width
              + destinationX
            ];

            for (
              Int16 column = static_cast<Int16>(width - 1);
              column >= 0;
              --column
            ) {
              destination[column] = source[column];
            }
          }
        } else {
          for (
            UInt16 row = 0;
            row < height;
            ++row
          ) {
            UInt32* source = &buffer[
              static_cast<UInt32>(sourceY + row) * _width
              + sourceX
            ];
            UInt32* destination = &buffer[
              static_cast<UInt32>(destinationY + row) * _width
              + destinationX
            ];

            for (
              UInt16 column = 0;
              column < width;
              ++column
            ) {
              destination[column] = source[column];
            }
          }
        }
      }
    }
  }
}
