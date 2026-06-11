/**
 * @file Include/Quantum/UI/Bezel.hpp
 * @brief Declares @ref @QUI::BezelLayer and @ref @QUI::Bezel.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Canvas.hpp"
#include "FrameT.hpp"
#include "IDecoration.hpp"
#include "Rectangle.hpp"

namespace Quantum::UI {
  /**
   * @brief A single layer of a bezel with per-side colors and a
   *        thickness.
   */
  struct BezelLayer {
    /**
     * @brief Per-side colors (Top, Left, Right, Bottom).
     */
    FrameT<UInt32> Colors = { 0, 0, 0, 0 };

    /**
     * @brief Thickness of this layer in pixels.
     */
    UInt8 Thickness = 0;
  };

  /**
   * @brief Maximum number of layers in a @ref Bezel.
   */
  static constexpr Size MaxBezelLayers = 8;

  /**
   * @brief A multi-layer beveled-edge decoration with per-side colors.
   *
   * Implements @ref IDecoration. Each layer draws four edge strips
   * around the content, from outermost (index 0) to innermost. Layers
   * with @ref BezelLayer::Thickness of 0 are skipped.
   */
  struct Bezel : public IDecoration {
    /**
     * @brief The layers, drawn from outermost (index 0) to innermost.
     */
    BezelLayer Layers[MaxBezelLayers] = {};

    /**
     * @brief Number of active layers.
     */
    Size LayerCount = 0;

    /**
     * @brief Appends a layer and returns a reference to it.
     */
    BezelLayer& AddLayer();

    /**
     * @brief Appends a uniform-color layer (same color on all sides).
     * @param color The color for all four sides.
     * @param thickness The layer thickness in pixels.
     * @return Reference to the added layer.
     */
    BezelLayer& AddLayer(UInt32 color, UInt8 thickness);

    /**
     * @brief Appends a two-tone layer (light top/left, dark
     *        bottom/right).
     * @param lightColor The top and left color.
     * @param darkColor The bottom and right color.
     * @param thickness The layer thickness in pixels.
     * @return Reference to the added layer.
     */
    BezelLayer& AddLayer(
      UInt32 lightColor,
      UInt32 darkColor,
      UInt8 thickness
    );

    /**
     * @brief Returns the total thickness of all layers combined.
     */
    UInt16 GetTotalThickness() const;

    void Draw(
      Canvas& canvas,
      Rectangle contentRectangle
    ) override;

    Rectangle ExpandBounds(
      Rectangle contentRectangle
    ) const override;
  };
}
