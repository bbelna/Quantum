/**
 * @file Include/Quantum/UI/Element.hpp
 * @brief Declares @ref @QUI::Element.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Canvas.hpp"
#include "Drawable.hpp"

namespace Quantum::App { class Window; }
namespace Quantum::Input { struct InputEvent; }

namespace Quantum::UI {
  class ClickTarget;

  /**
   * @brief Abstract base class for UI elements that render into a @ref Canvas.
   *
   * Elements can be added to a @ref Quantum::App via `Window::Add`,
   * which registers them for automatic event routing and drawing. The window
   * maintains an intrusive singly-linked list of elements using the @ref _next
   * pointer.
   */
  class Element : public Drawable {
    public:
      /**
       * @brief Constructs an element that renders into the given canvas.
       * @param canvas The rendering canvas.
       */
      explicit Element(Canvas& canvas) : _canvas(canvas) {}

      /**
       * @brief Destroys the element, removing it from the owning window's
       *        element list if it was added to one.
       */
      virtual ~Element();

      /**
       * @brief Renders the element into its surface.
       */
      virtual void Draw() = 0;

      /**
       * @brief Returns this element as a @ref ClickTarget if it handles
       *        mouse input; `nullptr` otherwise.
       *
       * Override in subclasses that inherit @ref ClickTarget (e.g. Button,
       * TextInput) so the owning window can route mouse events automatically.
       */
      virtual ClickTarget* AsClickTarget() { return nullptr; }

      /**
       * @brief Returns whether this element accepts keyboard input.
       *
       * Override in subclasses that handle keyboard events (e.g. TextInput,
       * Console) so the owning window can route keyboard focus.
       */
      virtual bool AcceptsKeyboard() const { return false; }

      /**
       * @brief Processes a keyboard input event.
       * @param event The input event to process.
       * @return `true` if the event was consumed; `false` otherwise.
       *
       * Called by the owning window when this element has keyboard focus.
       */
      virtual bool ProcessKeyEvent(const Input::InputEvent&) { return false; }

      /**
       * @brief Called by the owning window when this element gains or loses
       *        keyboard focus.
       * @param focused `true` if gaining focus; `false` if losing it.
       *
       * Override in subclasses that have visual focus indicators (e.g.
       * TextInput's cursor).
       */
      virtual void OnFocusChanged(bool focused) { (void)focused; }

      /**
       * @brief Returns the bounding rectangle of this element in
       *        content-pixel coordinates.
       *
       * Used for dirty-region invalidation. The default implementation
       * returns the full surface area. Subclasses with known bounds should
       * override for more precise invalidation.
       */
      virtual Geometry2D::Rectangle GetBounds() const;

      /**
       * @brief Marks this element's bounds as dirty on the owning window,
       *        causing the compositor to redraw the affected region.
       *
       * No-op if the element has not been added to a window.
       */
      void Invalidate();

      /**
       * @brief Returns the next element in the window's linked list.
       */
      Element* GetNext() const { return _next; }

    protected:
      /**
       * @brief The canvas this element renders into.
       */
      Canvas& _canvas;

    private:
      friend class Quantum::App::Window;

      /**
       * @brief Next element in the window's intrusive linked list.
       */
      Element* _next = nullptr;

      /**
       * @brief Back-pointer to the owning window, set by Window::Add.
       */
      App::Window* _window = nullptr;
  };
}
