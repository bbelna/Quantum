/**
 * @file Include/Quantum/UI/ClickTarget.hpp
 * @brief Declares @ref @QUI::ClickTarget.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::UI {
  /**
   * @brief Callback type invoked when a click completes.
   * @param userData Opaque pointer passed through from
   *                 @ref ClickTarget::SetOnClick.
   * 
   * A click is *complete* when the user presses the mouse button down on a
   * target, then releases the button while the cursor is still inside the
   * target's bounds.
   */
  using ClickCallback = void (*)(void* userData);

  /**
   * @brief Base class for rectangular UI elements that track mouse capture
   *        and deliver click events.
   */
  class ClickTarget {
    public:
      /**
       * @brief Creates a new @ref ClickTarget instance with the given
       *        bounds.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param width The width in pixels.
       * @param height The height in pixels.
       */
      ClickTarget(Int16 x, Int16 y, UInt16 width, UInt16 height);

      /**
       * @brief Destroys this @ref ClickTarget instance.
       */
      virtual ~ClickTarget() = default;

      /**
       * @brief Tests whether the given point is inside this target's bounds.
       * @param px The x-coordinate to test.
       * @param py The y-coordinate to test.
       * @return `true` if the point is inside; `false` otherwise.
       */
      bool HitTest(Int16 px, Int16 py) const;

      /**
       * @brief Handles a mouse-down event. If the point is inside this
       *        target, captures the mouse and enters the pressed state.
       * @param x The content-relative x-coordinate.
       * @param y The content-relative y-coordinate.
       * @return `true` if the target captured the mouse (point was inside);
       *         `false` otherwise.
       */
      virtual bool HandleMouseDown(Int16 x, Int16 y);

      /**
       * @brief Handles a mouse-up event. If this target has capture and the
       *        point is inside, fires the click callback.
       * @param x The content-relative x-coordinate.
       * @param y The content-relative y-coordinate.
       * @return `true` if a click was fired; `false` otherwise.
       */
      virtual bool HandleMouseUp(Int16 x, Int16 y);

      /**
       * @brief Handles a mouse-move event while captured. Updates the
       *        pressed state based on whether the cursor is inside this target.
       * @param x The content-relative x-coordinate.
       * @param y The content-relative y-coordinate.
       */
      virtual void HandleMouseMove(Int16 x, Int16 y);

      /**
       * @brief Returns whether this target currently has mouse capture.
       */
      bool HasCapture() const { return _captured; }

      /**
       * @brief Sets the click callback for this target.
       * @param callback The function to call on click.
       * @param userData Opaque pointer forwarded to the callback.
       */
      void SetOnClick(ClickCallback callback, void* userData = nullptr);

    protected:
      /**
       * @brief The x-coordinate of the target's top-left corner.
       */
      Int16 _x;

      /**
       * @brief The y-coordinate of the target's top-left corner.
       */
      Int16 _y;

      /**
       * @brief The width of the target in pixels.
       */
      UInt16 _width;

      /**
       * @brief The height of the target in pixels.
       */
      UInt16 _height;

      /**
       * @brief Called when the pressed visual state should change.
       *        Subclasses override this to update their appearance.
       * @param pressed `true` if entering the pressed state; `false` otherwise.
       */
      virtual void OnPressedChanged(bool pressed) = 0;

    private:
      /**
       * @brief Indicates whether the target currently has mouse capture
       *        (between a mouse-down inside the target and the corresponding
       *        mouse-up).
       */
      bool _captured = false;

      /**
       * @brief The click callback function to invoke on a completed click.
       */
      ClickCallback _onClick = nullptr;

      /**
       * @brief Opaque pointer to forward to the click callback.
       */
      void* _onClickUserData = nullptr;
  };
}
