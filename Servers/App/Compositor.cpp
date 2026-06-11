/**
 * @file Servers/App/Compositor.cpp
 * @brief Implements @ref @QAppSrv::Compositor.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <AppServerTypes.hpp>

#include "Compositor.hpp"
#include "Dock/Dock.hpp"
#include "Overlays/OverlayManager.hpp"
#include "Windows/WindowManager.hpp"

namespace Quantum::Servers::App {
  Compositor::Compositor() {}

  void Compositor::Init(
    GraphicsClient* graphics,
    WindowManager* windowManager,
    Sync::Mutex* stateLock,
    KernelClient* kernel,
    UInt16 screenWidth,
    UInt16 screenHeight,
    UInt8 bufferBpp,
    UInt16 bufferWidth,
    void* compositeBuffer,
    void* sharedBuffer,
    bool directFramebuffer,
    bool hasHardwareCursor
  ) {
    _graphics = graphics;
    _windowManager = windowManager;
    _stateLock = stateLock;
    _kernel = kernel;
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;
    _bufferBpp = bufferBpp;
    _bufferWidth = bufferWidth;
    _compositeBuffer = compositeBuffer;
    _sharedBuffer = sharedBuffer;
    _directFramebuffer = directFramebuffer;
    _hasHardwareCursor = hasHardwareCursor;
  }

  void Compositor::BufferFillRectangle(Rectangle rect, UInt32 color) {
    Rectangle screen(0, 0, _screenWidth, _screenHeight);
    Rectangle clamped = rect.Intersect(screen);

    if (clamped.IsEmpty()) return;

    Int16 x = clamped.Origin.X;
    Int16 y = clamped.Origin.Y;
    UInt16 fw = clamped.Dimensions.Width;
    Int16 y2 = clamped.GetBottom();

    if (_bufferBpp == 2) {
      UInt8 alpha16 = Theme::EnableTranslucency
        ? static_cast<UInt8>((color >> 24) & 0xFF)
        : static_cast<UInt8>(0xFF);

      if (alpha16 == 0xFF) {
        UInt16 c = ToRGB565(color);
        UInt32 packed = static_cast<UInt32>(c) | (static_cast<UInt32>(c) << 16);
        UInt16* buf = static_cast<UInt16*>(_compositeBuffer);

        for (Int16 row = y; row < y2; ++row) {
          UInt16* dst = &buf[static_cast<UInt32>(row) * _bufferWidth + x];
          UInt16 pairs = fw >> 1;
          UInt32* dst32 = reinterpret_cast<UInt32*>(dst);

          for (UInt16 p = 0; p < pairs; ++p) dst32[p] = packed;

          UInt16 col = static_cast<UInt16>(pairs << 1);

          if (col < fw) dst[col] = c;
        }
      } else if (alpha16 > 0) {
        UInt16* buf = static_cast<UInt16*>(_compositeBuffer);

        for (Int16 row = y; row < y2; ++row) {
          UInt16* dst = &buf[static_cast<UInt32>(row) * _bufferWidth + x];

          for (UInt16 col = 0; col < fw; ++col) {
            UInt32 dstARGB = Color::FromRGB565(dst[col]);
            UInt32 blended = Color::BlendARGB(color, dstARGB, alpha16);

            dst[col] = Color::ToRGB565(blended);
          }
        }
      }
    } else {
      UInt32* buf = static_cast<UInt32*>(_compositeBuffer);
      UInt8 alpha = Theme::EnableTranslucency
        ? static_cast<UInt8>((color >> 24) & 0xFF)
        : static_cast<UInt8>(0xFF);

      if (alpha == 0xFF) {
        UInt32 opaqueColor = color | 0xFF000000;

        for (Int16 row = y; row < y2; ++row) {
          void* dst = &buf[static_cast<UInt32>(row) * _bufferWidth + x];
          UInt32 count = fw;

          asm volatile(
            "cld\n"
            "rep stosl"
            : "+D"(dst), "+c"(count)
            : "a"(opaqueColor)
            : "memory"
          );
        }
      } else if (alpha > 0) {
        for (Int16 row = y; row < y2; ++row) {
          UInt32* dst = &buf[
            static_cast<UInt32>(row) * _bufferWidth + x
          ];

          for (UInt16 col = 0; col < fw; ++col) {
            dst[col] = Color::BlendARGB(
              color, dst[col], alpha
            );
          }
        }
      }
    }
  }

  void Compositor::InitBuffer(
    void* buffer, UInt32 pixelCount, UInt32 argbColor
  ) {
    if (!buffer || pixelCount == 0) return;

    if (_bufferBpp == 2) {
      UInt16 c = ToRGB565(argbColor);
      UInt16* buf = static_cast<UInt16*>(buffer);

      for (UInt32 p = 0; p < pixelCount; ++p) {
        buf[p] = c;
      }
    } else {
      UInt32* buf = static_cast<UInt32*>(buffer);

      for (UInt32 p = 0; p < pixelCount; ++p) {
        buf[p] = argbColor;
      }
    }
  }

  void Compositor::BufferRenderGlyph(
    Int16 x,
    Int16 y,
    UInt8 ch,
    UInt32 fore,
    const Fonts::BitmapFont& font,
    bool bold
  ) {
    UInt8 gw = font.Width;
    UInt8 gh = font.Height;

    if (_bufferBpp == 2) {
      UInt16 c = ToRGB565(fore);
      UInt16* buf = static_cast<UInt16*>(_compositeBuffer);

      for (UInt8 row = 0; row < gh; ++row) {
        Int16 py = static_cast<Int16>(y + row);

        if (py < 0) continue;
        if (py >= static_cast<Int16>(_screenHeight)) break;

        UInt8 bits = font.GetRow(ch, row);

        if (bold) bits |= (bits >> 1);

        for (UInt8 col = 0; col < gw; ++col) {
          if ((bits >> (7 - col)) & 1) {
            Int16 px = static_cast<Int16>(x + col);

            if (px >= 0 && px < static_cast<Int16>(_screenWidth)) {
              buf[static_cast<UInt32>(py) * _bufferWidth + px] = c;
            }
          }
        }
      }
    } else {
      UInt32* buf = static_cast<UInt32*>(_compositeBuffer);

      for (UInt8 row = 0; row < gh; ++row) {
        Int16 py = static_cast<Int16>(y + row);

        if (py < 0) continue;
        if (py >= static_cast<Int16>(_screenHeight)) break;

        UInt8 bits = font.GetRow(ch, row);

        if (bold) bits |= (bits >> 1);

        for (UInt8 col = 0; col < gw; ++col) {
          if ((bits >> (7 - col)) & 1) {
            Int16 px = static_cast<Int16>(x + col);

            if (px >= 0 && px < static_cast<Int16>(_screenWidth)) {
              buf[static_cast<UInt32>(py) * _bufferWidth + px] = fore;
            }
          }
        }
      }
    }
  }

  void Compositor::BufferBlit(
    Int16 srcX,
    Int16 srcY,
    Int16 dstX,
    Int16 dstY,
    UInt16 w,
    UInt16 h
  ) {
    if (!_compositeBuffer || w == 0 || h == 0) return;

    Int32 sw = static_cast<Int32>(_screenWidth);
    Int32 sh = static_cast<Int32>(_screenHeight);
    Int32 sx = srcX, sy = srcY, dx = dstX, dy = dstY;
    Int32 bw = w, bh = h;

    // clip source left/top
    if (sx < 0) { bw += sx; dx -= sx; sx = 0; }
    if (sy < 0) { bh += sy; dy -= sy; sy = 0; }

    // clip source right/bottom
    if (sx + bw > sw) bw = sw - sx;
    if (sy + bh > sh) bh = sh - sy;

    // clip destination left/top
    if (dx < 0) { bw += dx; sx -= dx; dx = 0; }
    if (dy < 0) { bh += dy; sy -= dy; dy = 0; }

    // clip destination right/bottom
    if (dx + bw > sw) bw = sw - dx;
    if (dy + bh > sh) bh = sh - dy;

    if (bw <= 0 || bh <= 0) return;

    bool reverseRows = (dy > sy) || (dy == sy && dx > sx);
    bool reverseCols = (dx > sx);

    if (_bufferBpp == 2) {
      UInt16* buf = static_cast<UInt16*>(_compositeBuffer);

      if (reverseRows) {
        for (Int32 row = bh - 1; row >= 0; --row) {
          UInt16* s = &buf[(sy + row) * _bufferWidth + sx];
          UInt16* d = &buf[(dy + row) * _bufferWidth + dx];

          if (reverseCols) {
            for (Int32 col = bw - 1; col >= 0; --col) d[col] = s[col];
          } else {
            void* dd = d;
            const void* ss = s;
            UInt32 cnt = static_cast<UInt32>(bw);

            asm volatile(
              "cld\n"
              "rep movsw"
              : "+D"(dd), "+S"(ss), "+c"(cnt)
              :
              : "memory"
            );
          }
        }
      } else {
        for (Int32 row = 0; row < bh; ++row) {
          UInt16* s = &buf[(sy + row) * _bufferWidth + sx];
          UInt16* d = &buf[(dy + row) * _bufferWidth + dx];

          void* dd = d;
          const void* ss = s;
          UInt32 cnt = static_cast<UInt32>(bw);

          asm volatile(
            "cld\n"
            "rep movsw"
            : "+D"(dd), "+S"(ss), "+c"(cnt)
            :
            : "memory"
          );
        }
      }
    } else {
      UInt32* buf = static_cast<UInt32*>(_compositeBuffer);

      if (reverseRows) {
        for (Int32 row = bh - 1; row >= 0; --row) {
          UInt32* s = &buf[(sy + row) * _bufferWidth + sx];
          UInt32* d = &buf[(dy + row) * _bufferWidth + dx];

          if (reverseCols) {
            for (Int32 col = bw - 1; col >= 0; --col) d[col] = s[col];
          } else {
            void* dd = d;
            const void* ss = s;
            UInt32 cnt = static_cast<UInt32>(bw);

            asm volatile(
              "cld\n"
              "rep movsl"
              : "+D"(dd), "+S"(ss), "+c"(cnt)
              :
              : "memory"
            );
          }
        }
      } else {
        for (Int32 row = 0; row < bh; ++row) {
          UInt32* s = &buf[(sy + row) * _bufferWidth + sx];
          UInt32* d = &buf[(dy + row) * _bufferWidth + dx];

          void* dd = d;
          const void* ss = s;
          UInt32 cnt = static_cast<UInt32>(bw);

          asm volatile(
            "cld\n"
            "rep movsl"
            : "+D"(dd), "+S"(ss), "+c"(cnt)
            :
            : "memory"
          );
        }
      }
    }
  }

  DrawContext Compositor::CreateDrawContext() {
    DrawContext ctx;

    ctx.UserData = this;
    ctx.BlitBuffer = nullptr;
    ctx.SkipContentBlit = false;

    if (_compositeBuffer) {
      ctx.FillRectangle = [](void* userData, Rectangle rect, UInt32 color) {
        static_cast<Compositor*>(userData)->BufferFillRectangle(rect, color);
      };

      ctx.RenderText = [](
        void* userData,
        Point origin,
        const char* text,
        Size length,
        UInt32 color,
        UInt16 fontWeight
      ) {
        auto* compositor = static_cast<Compositor*>(userData);
        bool bold = fontWeight > 500;

        const auto& font = Window::GetTitleFont();
        Int16 cx = origin.X;

        for (Size i = 0; i < length && text[i]; ++i) {
          UInt8 code = static_cast<UInt8>(text[i]);

          compositor->BufferRenderGlyph(cx, origin.Y, code, color, font, bold);

          cx = static_cast<Int16>(cx + font.GetAdvance(code));
        }
      };

      ctx.BlitBuffer = [](
        void* userData,
        Int16 dstX,
        Int16 dstY,
        const void* src,
        UInt16 w,
        UInt16 h,
        UInt16 srcStride,
        UInt8 srcBpp
      ) {
        auto* compositor = static_cast<Compositor*>(userData);

        if (!compositor->_compositeBuffer) return;

        for (UInt16 row = 0; row < h; ++row) {
          Int16 py = static_cast<Int16>(dstY + row);

          if (py < 0) continue;
          if (py >= static_cast<Int16>(compositor->_screenHeight)) break;

          Int16 sx = 0;
          Int16 dx = dstX;
          UInt16 cw = w;

          if (dx < 0) {
            sx = static_cast<Int16>(-dx);
            cw = static_cast<UInt16>(cw - sx);
            dx = 0;
          }

          if (dx + cw > compositor->_screenWidth) {
            cw = static_cast<UInt16>(compositor->_screenWidth - dx);
          }

          if (cw == 0) continue;

          UInt32 srcOffset = static_cast<UInt32>(row) * srcStride + sx;
          UInt32 dstOffset
            = static_cast<UInt32>(py) * compositor->_bufferWidth
            + dx;

          if (srcBpp == 2 && compositor->_bufferBpp == 2) {
            const UInt16* srcRow
              = &static_cast<const UInt16*>(src)[srcOffset];
            UInt16* dstRow = &static_cast<UInt16*>(
              compositor->_compositeBuffer
            )[dstOffset];

            void* d = dstRow;
            const void* s = srcRow;
            UInt32 count = cw;

            asm volatile(
              "cld\n"
              "rep movsw"
              : "+D"(d), "+S"(s), "+c"(count)
              :
              : "memory"
            );
          } else if (srcBpp == 4 && compositor->_bufferBpp == 2) {
            const UInt32* srcRow
              = &static_cast<const UInt32*>(src)[srcOffset];
            UInt16* dstRow = &static_cast<UInt16*>(
              compositor->_compositeBuffer
            )[dstOffset];

            for (UInt16 col = 0; col < cw; ++col) {
              if (Theme::EnableTranslucency) {
                UInt8 alpha = static_cast<UInt8>(
                  (srcRow[col] >> 24) & 0xFF
                );

                if (alpha == 0xFF) {
                  dstRow[col] = Color::ToRGB565(srcRow[col]);
                } else if (alpha > 0) {
                  UInt32 dstARGB = Color::FromRGB565(dstRow[col]);
                  UInt32 blended = Color::BlendARGB(
                    srcRow[col], dstARGB, alpha
                  );

                  dstRow[col] = Color::ToRGB565(blended);
                }
              } else {
                dstRow[col] = Color::ToRGB565(srcRow[col]);
              }
            }
          } else {
            const UInt32* srcRow
              = &static_cast<const UInt32*>(src)[srcOffset];
            UInt32* dstRow = &static_cast<UInt32*>(
              compositor->_compositeBuffer
            )[dstOffset];

            if (Theme::EnableTranslucency) {
              for (UInt16 col = 0; col < cw; ++col) {
                UInt8 alpha = static_cast<UInt8>(
                  (srcRow[col] >> 24) & 0xFF
                );

                if (alpha == 0xFF) {
                  dstRow[col] = srcRow[col];
                } else if (alpha > 0) {
                  dstRow[col] = Color::BlendARGB(
                    srcRow[col], dstRow[col], alpha
                  );
                }
              }
            } else {
              void* d = dstRow;
              const void* s = srcRow;
              UInt32 count = cw;

              asm volatile(
                "cld\n"
                "rep movsl"
                : "+D"(d), "+S"(s), "+c"(count)
                :
                : "memory"
              );
            }
          }
        }
      };
    } else {
      ctx.FillRectangle = [](
        void* userData, Rectangle rect, UInt32 color
      ) {
        static_cast<Compositor*>(userData)->_graphics->ClippedFillRectangle(
          rect, color
        );
      };

      ctx.RenderText = [](
        void* userData, Point origin,
        const char* text, Size length, UInt32 color,
        UInt16 fontWeight
      ) {
        const auto& font = Window::GetTitleFont();

        static_cast<Compositor*>(userData)->_graphics->DrawText(
          origin.X, origin.Y, text, length, color, fontWeight,
          font.Width, font.Height
        );
      };
    }

    return ctx;
  }

  void Compositor::CompositeStrip(Rectangle strip, DrawContext& ctx) {
    Rectangle screen(0, 0, _screenWidth, _screenHeight);
    Rectangle clamped = strip.Intersect(screen);

    if (clamped.IsEmpty()) return;

    // find the frontmost window that fully covers this strip; all windows
    // behind it are invisible and can be skipped
    auto& windows = _windowManager->GetWindows();
    PathNode<Window*>* coverNode = nullptr;

    for (
      auto* node = windows.GetTail();
      node;
      node = node->GetPrevious()
    ) {
      if (node->GetValue()->IsMinimized()) continue;
      if (!node->GetValue()->IsContentReady()) continue;

      if (node->GetValue()->GetChromeFrame().Contains(clamped)) {
        coverNode = node;

        break;
      }
    }

    if (!coverNode) {
      BufferFillRectangle(clamped, BackgroundColor);

      // draw menu bar shadow between background and windows so it
      // appears below windows, not on top of them
      UInt16 menuBarBottom = _overlayManager
        ? _overlayManager->GetTopOverlayBottom() : 0;

      if (
        Theme::EnableShadows
        && menuBarBottom > 0 && _compositeBuffer
      ) {
        UInt8 shadowSize = Theme::ShadowSize;

        // menu bar shadow drops straight down (no offset), starting
        // immediately at the menu bar bottom edge
        for (UInt8 layer = 0; layer < shadowSize; ++layer) {
          UInt8 alpha = static_cast<UInt8>(
            Theme::ShadowAlpha
            * static_cast<UInt32>(shadowSize - layer)
            * (shadowSize - layer)
            / (static_cast<UInt32>(shadowSize) * shadowSize)
          );
          UInt32 shadowColor = static_cast<UInt32>(alpha) << 24;

          Rectangle shadowRow(
            clamped.Origin.X,
            static_cast<Int16>(menuBarBottom + layer),
            clamped.Dimensions.Width,
            1
          );
          Rectangle visible = shadowRow.Intersect(clamped);

          if (!visible.IsEmpty()) {
            BufferFillRectangle(visible, shadowColor);
          }
        }
      }
    }

    auto* startNode = coverNode ? coverNode : windows.GetHead();

    for (auto* node = startNode; node; node = node->GetNext()) {
      if (node->GetValue()->IsMinimized()) continue;
      if (!node->GetValue()->IsContentReady()) continue;
      if (node->GetValue()->GetVisualBounds().Intersect(clamped).IsEmpty()) {
        continue;
      }

      node->GetValue()->DrawClipped(clamped, ctx);
    }

    // render overlays above all windows
    if (_overlayManager) {
      _overlayManager->DrawOverlays(clamped, ctx);
    }

    // render dock at bottom of screen
    if (_dock) {
      _dock->DrawClipped(clamped, ctx);
    }
  }

  void Compositor::Damage(Rectangle rect) {
    if (rect.IsEmpty()) return;

    Rectangle screen(0, 0, _screenWidth, _screenHeight);
    Rectangle clamped = rect.Intersect(screen);

    if (clamped.IsEmpty()) return;

    if (_dirtyRectCount < MaxDamageRects) {
      _dirtyRects[_dirtyRectCount++] = clamped;
      return;
    }

    // Array full, find the pair whose union has the smallest area and merge
    Size bestI = 0, bestJ = 1;
    UInt32 bestArea = 0xFFFFFFFF;

    for (Size i = 0; i < _dirtyRectCount; ++i) {
      for (Size j = i + 1; j < _dirtyRectCount; ++j) {
        Rectangle u = _dirtyRects[i].Union(_dirtyRects[j]);
        UInt32 area = static_cast<UInt32>(u.Dimensions.Width) *
                      static_cast<UInt32>(u.Dimensions.Height);

        if (area < bestArea) {
          bestArea = area;
          bestI = i;
          bestJ = j;
        }
      }
    }

    _dirtyRects[bestI] = _dirtyRects[bestI].Union(_dirtyRects[bestJ]);
    _dirtyRects[bestJ] = _dirtyRects[_dirtyRectCount - 1];
    _dirtyRectCount--;

    // Now there's room, append
    _dirtyRects[_dirtyRectCount++] = clamped;
  }

  void Compositor::ClearDamage() {
    _dirtyRectCount = 0;
  }

  void Compositor::SignalRender() {
    _renderSignal = 1;
    _kernel->FutexWake(&_renderSignal, 1);
  }

  void Compositor::WaitForCompositing() {
    while (_compositing) {
      _stateLock->Unlock();
      _kernel->FutexWait(
        reinterpret_cast<volatile UInt32*>(&_compositing), 1
      );
      _stateLock->Lock();
    }
  }

  void Compositor::RenderThreadEntry(UInt32 arg) {
    KernelClient kernel;
    kernel.SetThreadPriority(
      static_cast<UInt32>(Kernel::ABI::Thread::Priority::DisplayServerRender)
    );

    auto* compositor = reinterpret_cast<Compositor*>(arg);

    compositor->RenderLoop();
  }

  void Compositor::RenderLoop() {
    for (;;) {
      _stateLock->Lock();

      _windowManager->ProcessDeleteQueue();

      // rebuild dock if window state changed
      if (_dock && _dock->IsDirty()) {
        _dock->Rebuild();
        Damage(_dock->GetFrame());
      }

      if (_dirtyRectCount == 0) {
        _renderSignal = 0;
        _stateLock->Unlock();

        _kernel->FutexWait(&_renderSignal, 0);

        continue;
      }

      Rectangle dirtyRects[MaxDamageRects];
      Size dirtyCount = _dirtyRectCount;

      for (Size i = 0; i < dirtyCount; ++i) {
        dirtyRects[i] = _dirtyRects[i];
      }

      ClearDamage();

      _renderSignal = 0;

      // release the lock now, the dirty rects are snapshotted and damage
      // is cleared
      // the main thread can resume processing mouse/input events and
      // accumulating new damage while we composite
      _compositing = 1;

      _stateLock->Unlock();

      // when _compositeBuffer == _sharedBuffer (the common case), a deferred
      // flush from the inline resize path may still be in-flight - the
      // graphics server reads directly from this buffer;  wait for it to
      // finish before we overwrite any pixels
      _graphics->WaitForPendingFlush();

      DrawContext ctx = CreateDrawContext();

      if (_compositeBuffer) {
        for (Size d = 0; d < dirtyCount; ++d) {
          CompositeStrip(dirtyRects[d], ctx);

          _graphics->FlushBackBuffer(
            static_cast<UInt16>(dirtyRects[d].Origin.X),
            static_cast<UInt16>(dirtyRects[d].Origin.Y),
            dirtyRects[d].Dimensions.Width,
            dirtyRects[d].Dimensions.Height,
            d == dirtyCount - 1
          );
        }
      } else {
        if (!_hasHardwareCursor) _graphics->ShowCursor(false);

        _graphics->BeginBatch();

        for (Size d = 0; d < dirtyCount; ++d) {
          Rectangle dr = dirtyRects[d];

          _graphics->ClippedFillRectangle(dr, BackgroundColor);

          auto& windows = _windowManager->GetWindows();

          for (
            auto* node = windows.GetHead();
            node;
            node = node->GetNext()
          ) {
            if (node->GetValue()->GetVisualBounds().Intersect(dr).IsEmpty()) {
              continue;
            }

            node->GetValue()->DrawClipped(dr, ctx);
          }
        }

        _graphics->EndBatch();

        if (!_hasHardwareCursor) _graphics->ShowCursor(true);
      }

      _compositing = 0;
      _kernel->FutexWake(
        reinterpret_cast<volatile UInt32*>(&_compositing), 1
      );
    }
  }

  void Compositor::RenderImmediate() {
    if (_dirtyRectCount == 0) return;

    DrawContext ctx = CreateDrawContext();

    if (_compositeBuffer) {
      for (Size d = 0; d < _dirtyRectCount; ++d) {
        Rectangle dr = _dirtyRects[d];

        // find the frontmost window that fully covers this dirty rect
        // all windows behind it are invisible for this rect and can be
        // skipped, this avoids redundant compositing of windows hidden
        // behind the topmost opaque cover (the Haiku clipping approach)
        auto& windows = _windowManager->GetWindows();
        PathNode<Window*>* coverNode = nullptr;

        for (
          auto* node = windows.GetTail();
          node;
          node = node->GetPrevious()
        ) {
          if (node->GetValue()->GetChromeFrame().Contains(dr)) {
            coverNode = node;

            break;
          }
        }

        if (!coverNode) {
          BufferFillRectangle(dr, BackgroundColor);

          // menu bar shadow (same as CompositeStrip)
          UInt16 menuBarBottom = _overlayManager
            ? _overlayManager->GetTopOverlayBottom() : 0;

          if (Theme::EnableShadows
              && menuBarBottom > 0 && _compositeBuffer) {
            UInt8 shadowSize = Theme::ShadowSize;

            for (UInt8 layer = 0; layer < shadowSize; ++layer) {
              UInt8 alpha = static_cast<UInt8>(
                Theme::ShadowAlpha
            * static_cast<UInt32>(shadowSize - layer)
            * (shadowSize - layer)
            / (static_cast<UInt32>(shadowSize) * shadowSize)
              );
              UInt32 shadowColor = static_cast<UInt32>(alpha) << 24;

              Rectangle shadowRow(
                dr.Origin.X,
                static_cast<Int16>(menuBarBottom + layer),
                dr.Dimensions.Width,
                1
              );
              Rectangle visible = shadowRow.Intersect(dr);

              if (!visible.IsEmpty()) {
                BufferFillRectangle(visible, shadowColor);
              }
            }
          }
        }

        auto* startNode = coverNode ? coverNode : windows.GetHead();

        for (auto* node = startNode; node; node = node->GetNext()) {
          if (node->GetValue()->GetVisualBounds().Intersect(dr).IsEmpty()) {
            continue;
          }

          node->GetValue()->DrawClipped(dr, ctx);
        }
      }

      // flush each dirty rect individually rather than their bounding-box
      // union
      // avoids pushing dead pixels through the PCI bus when rects are
      // scattered (e.g. right + bottom resize strips)
      for (Size d = 0; d < _dirtyRectCount; ++d) {
        Rectangle dr = _dirtyRects[d];

        _graphics->FlushBackBuffer(
          static_cast<UInt16>(dr.Origin.X),
          static_cast<UInt16>(dr.Origin.Y),
          dr.Dimensions.Width,
          dr.Dimensions.Height,
          d == _dirtyRectCount - 1
        );
      }
    } else {
      if (!_hasHardwareCursor) _graphics->ShowCursor(false);

      _graphics->BeginBatch();

      for (Size d = 0; d < _dirtyRectCount; ++d) {
        Rectangle dr = _dirtyRects[d];

        _graphics->ClippedFillRectangle(dr, BackgroundColor);

        auto& windows = _windowManager->GetWindows();

        for (
          auto* node = windows.GetHead();
          node;
          node = node->GetNext()
        ) {
          if (node->GetValue()->GetVisualBounds().Intersect(dr).IsEmpty()) {
            continue;
          }

          node->GetValue()->DrawClipped(dr, ctx);
        }
      }

      _graphics->EndBatch();

      if (!_hasHardwareCursor) _graphics->ShowCursor(true);
    }

    ClearDamage();
  }

  void* Compositor::GetCompositeBuffer() const {
    return _compositeBuffer;
  }

  void* Compositor::GetSharedBuffer() const {
    return _sharedBuffer;
  }

  UInt8 Compositor::GetBufferBpp() const {
    return _bufferBpp;
  }

  UInt16 Compositor::GetBufferWidth() const {
    return _bufferWidth;
  }

  bool Compositor::IsDirectFramebuffer() const {
    return _directFramebuffer;
  }

  volatile UInt32& Compositor::GetRenderSignal() {
    return _renderSignal;
  }

  UInt16 Compositor::ToRGB565(UInt32 argb) {
    return Color::ToRGB565(argb);
  }

}
