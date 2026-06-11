/**
 * @file UI/Canvas.cpp
 * @brief Implements @ref @QUI::Canvas.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/UI/Canvas.hpp>

#include "UITypes.hpp"

namespace Quantum::UI {
  Canvas::Canvas(
    UInt32* pixels,
    UInt16 width,
    UInt16 height,
    UInt16 stride
  ) :
    _pixels(pixels),
    _width(width),
    _height(height),
    _stride(stride),
    _bytesPerPixel(4),
    _painter(*this)
  {
  }

  Canvas::Canvas(
    void* pixels,
    UInt16 width,
    UInt16 height,
    UInt16 stride,
    UInt8 bytesPerPixel
  ) :
    _pixels(pixels),
    _width(width),
    _height(height),
    _stride(stride),
    _bytesPerPixel(bytesPerPixel),
    _painter(*this)
  {
  }

  Canvas::Canvas() :
    _pixels(nullptr),
    _width(0),
    _height(0),
    _stride(0),
    _bytesPerPixel(4),
    _painter(*this)
  {
  }

  Canvas::Canvas(Canvas&& other) noexcept :
    _pixels(other._pixels),
    _width(other._width),
    _height(other._height),
    _stride(other._stride),
    _bytesPerPixel(other._bytesPerPixel),
    _painter(*this)
  {
    _painter._font = other._painter._font;
    other._pixels = nullptr;
    other._width = 0;
    other._height = 0;
  }

  Canvas& Canvas::operator=(Canvas&& other) noexcept {
    if (this != &other) {
      _pixels = other._pixels;
      _width = other._width;
      _height = other._height;
      _stride = other._stride;
      _bytesPerPixel = other._bytesPerPixel;
      _painter._font = other._painter._font;
      other._pixels = nullptr;
      other._width = 0;
      other._height = 0;
    }

    return *this;
  }

  UInt16 Canvas::ToRGB565(UInt32 argb) {
    return Color::ToRGB565(argb);
  }

  void Canvas::SetDimensions(UInt16 width, UInt16 height) {
    _width = width;
    _height = height;
  }

  void Canvas::Reset(
    void* pixels,
    UInt16 width,
    UInt16 height,
    UInt16 stride,
    UInt8 bytesPerPixel
  ) {
    _pixels = pixels;
    _width = width;
    _height = height;
    _stride = stride;
    _bytesPerPixel = bytesPerPixel;
  }
}
