/**
 * @file Include/Quantum/Core/Color.hpp
 * @brief Declares @ref Quantum::Core::Color.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

namespace Quantum::Core {
  /**
   * @brief Utility class for color conversion and alpha blending.
   */
  class Color {
    public:
      /**
       * @brief Blends a foreground ARGB32 color over a background using an
       *        8-bit alpha value.
       * @param foreground The foreground color in ARGB32 format.
       * @param background The background color in ARGB32 format.
       * @param alpha Blend factor (`0` = fully background,
       *              `255` = fully foreground).
       * @return The blended ARGB32 color.
       */
      static UInt32 BlendARGB(
        UInt32 foreground,
        UInt32 background,
        UInt8 alpha
      );

      /**
       * @brief Blends a foreground RGB565 color over a background using an
       *        8-bit alpha value.
       * @param foreground The foreground color in RGB565 format.
       * @param background The background color in RGB565 format.
       * @param alpha Blend factor (`0` = fully background,
       *              `255` = fully foreground).
       * @return The blended RGB565 color.
       */
      static UInt16 BlendRGB565(
        UInt16 foreground,
        UInt16 background,
        UInt8 alpha
      );

      /**
       * @brief Composites an ARGB32 foreground color over an existing
       *        background pixel using the foreground's own alpha channel.
       * @param foreground The foreground color with embedded alpha.
       * @param background The existing pixel to blend over.
       * @return The composited ARGB32 color.
       */
      static inline UInt32 Composite(UInt32 foreground, UInt32 background) {
        UInt8 alpha = static_cast<UInt8>((foreground >> 24) & 0xFF);

        return BlendARGB(foreground, background, alpha);
      }

      /**
       * @brief Alpha-aware "over" operator that preserves the output alpha
       *        channel for translucent compositing.
       * @param foreground Foreground color (alpha used as coverage).
       * @param glyphAlpha Glyph coverage (`0` = fully background,
       *                   `255` = fully foreground).
       * @param background Background color (alpha channel preserved in
       *                   output for uncovered regions).
       * @return The composited ARGB32 color with correct output alpha.
       *
       * Unlike @ref BlendARGB which forces opaque output, this function
       * computes the output alpha from the glyph coverage and the
       * background's existing alpha, suitable for rendering onto
       * translucent surfaces.
       */
      static UInt32 BlendOver(
        UInt32 foreground,
        UInt32 background,
        UInt8 glyphAlpha
      );

      /**
       * @brief Converts an RGB565 color to ARGB32.
       * @param rgb565 The color in RGB565 format.
       * @return The color in ARGB32 format (fully opaque).
       */
      static inline UInt32 FromRGB565(UInt16 rgb565) {
        UInt32 r5 = (rgb565 >> 11) & 0x1F;
        UInt32 g6 = (rgb565 >> 5) & 0x3F;
        UInt32 b5 = rgb565 & 0x1F;

        UInt8 red = static_cast<UInt8>((r5 << 3) | (r5 >> 2));
        UInt8 green = static_cast<UInt8>((g6 << 2) | (g6 >> 4));
        UInt8 blue = static_cast<UInt8>((b5 << 3) | (b5 >> 2));

        return 0xFF000000
          | (static_cast<UInt32>(red) << 16)
          | (static_cast<UInt32>(green) << 8)
          | blue;
      }

      /**
       * @brief Converts an ARGB32 color to RGB565.
       * @param argb The color in ARGB32 format.
       * @return The color in RGB565 format.
       */
      static inline UInt16 ToRGB565(UInt32 argb) {
        UInt8 red = static_cast<UInt8>((argb >> 16) & 0xFF);
        UInt8 green = static_cast<UInt8>((argb >> 8) & 0xFF);
        UInt8 blue = static_cast<UInt8>(argb & 0xFF);

        // when all channels are equal (grays), truncate uniformly
        // using the 5-bit quantization to avoid the green tint that
        // RGB565's asymmetric 5-6-5 bit depths produce on dark grays
        if (red == green && green == blue) {
          UInt16 r5 = static_cast<UInt16>(red >> 3);
          UInt16 g6 = static_cast<UInt16>(r5 << 1);

          return static_cast<UInt16>(
            (r5 << 11) | (g6 << 5) | r5
          );
        }

        return static_cast<UInt16>(
          (static_cast<UInt16>(red >> 3) << 11) |
          (static_cast<UInt16>(green >> 2) << 5) |
          (blue >> 3)
        );
      }

      /**
       * @brief Gamma-correct ARGB32 blend.
       * @param foreground The foreground color in ARGB32 format.
       * @param background The background color in ARGB32 format.
       * @param alpha Blend factor (`0` = fully background,
       *              `255` = fully foreground).
       * @return The blended ARGB32 color.
       */
      static UInt32 BlendGammaARGB(
        UInt32 foreground,
        UInt32 background,
        UInt8 alpha
      );

      /**
       * @brief Gamma-correct alpha-aware "over" operator.
       * @param foreground Foreground color.
       * @param background Background color (alpha preserved for uncovered
       *                   regions).
       * @param glyphAlpha Glyph coverage value.
       * @return The composited ARGB32 color with gamma-correct RGB
       *         blending.
       */
      static UInt32 BlendGammaOver(
        UInt32 foreground,
        UInt32 background,
        UInt8 glyphAlpha
      );

      /**
       * @brief Gamma-correct RGB565 blend.
       * @param foreground The foreground color in RGB565 format.
       * @param background The background color in RGB565 format.
       * @param alpha Blend factor.
       * @return The blended RGB565 color.
       */
      static inline UInt16 BlendGammaRGB565(
        UInt16 foreground,
        UInt16 background,
        UInt8 alpha
      ) {
        if (alpha == 255) return foreground;
        if (alpha == 0) return background;

        return ToRGB565(BlendGammaARGB(
          FromRGB565(foreground),
          FromRGB565(background),
          alpha
        ));
      }

      /**
       * @brief Subpixel (LCD) ARGB32 blend with per-channel coverage and
       *        gamma-correct interpolation.
       * @param foreground The foreground color in ARGB32 format.
       * @param background The background color in ARGB32 format.
       * @param coverageR Red subpixel coverage (`0`-`255`).
       * @param coverageG Green subpixel coverage (`0`-`255`).
       * @param coverageB Blue subpixel coverage (`0`-`255`).
       * @return The blended ARGB32 color.
       */
      static UInt32 BlendSubpixelARGB(
        UInt32 foreground,
        UInt32 background,
        UInt8 coverageR,
        UInt8 coverageG,
        UInt8 coverageB
      );

      /**
       * @brief Subpixel (LCD) RGB565 blend with per-channel coverage.
       * @param foreground The foreground color in RGB565 format.
       * @param background The background color in RGB565 format.
       * @param coverageR Red subpixel coverage (`0`-`255`).
       * @param coverageG Green subpixel coverage (`0`-`255`).
       * @param coverageB Blue subpixel coverage (`0`-`255`).
       * @return The blended RGB565 color.
       */
      static inline UInt16 BlendSubpixelRGB565(
        UInt16 foreground,
        UInt16 background,
        UInt8 coverageR,
        UInt8 coverageG,
        UInt8 coverageB
      ) {
        return ToRGB565(BlendSubpixelARGB(
          FromRGB565(foreground),
          FromRGB565(background),
          coverageR, coverageG, coverageB
        ));
      }

    private:
      /**
       * @brief Square root lookup table for delinearizing gamma 2.0 blended
       *        values.
       *
       * Entry @c i maps to `round(sqrt(i * 256 + 128))`, covering
       * the range of `linearSquared >> 8` values produced by
       * gamma-correct blending (0-65025 squared-space to 0-255 sRGB).
       */
      static constexpr UInt8 _sqrtLUT[256] = {
         11,  19,  25,  29,  33,  37,  40,  43,
         46,  49,  51,  54,  56,  58,  60,  62,
         64,  66,  68,  70,  72,  74,  75,  77,
         79,  80,  82,  83,  85,  86,  88,  89,
         91,  92,  93,  95,  96,  97,  99, 100,
        101, 103, 104, 105, 106, 107, 109, 110,
        111, 112, 113, 114, 115, 117, 118, 119,
        120, 121, 122, 123, 124, 125, 126, 127,
        128, 129, 130, 131, 132, 133, 134, 135,
        136, 137, 138, 139, 139, 140, 141, 142,
        143, 144, 145, 146, 147, 147, 148, 149,
        150, 151, 152, 153, 153, 154, 155, 156,
        157, 157, 158, 159, 160, 161, 161, 162,
        163, 164, 165, 165, 166, 167, 168, 168,
        169, 170, 171, 171, 172, 173, 174, 174,
        175, 176, 177, 177, 178, 179, 179, 180,
        181, 182, 182, 183, 184, 184, 185, 186,
        186, 187, 188, 188, 189, 190, 190, 191,
        192, 192, 193, 194, 194, 195, 196, 196,
        197, 198, 198, 199, 200, 200, 201, 202,
        202, 203, 203, 204, 205, 205, 206, 207,
        207, 208, 208, 209, 210, 210, 211, 211,
        212, 213, 213, 214, 214, 215, 216, 216,
        217, 217, 218, 219, 219, 220, 220, 221,
        221, 222, 223, 223, 224, 224, 225, 225,
        226, 227, 227, 228, 228, 229, 229, 230,
        231, 231, 232, 232, 233, 233, 234, 234,
        235, 235, 236, 237, 237, 238, 238, 239,
        239, 240, 240, 241, 241, 242, 242, 243,
        243, 244, 245, 245, 246, 246, 247, 247,
        248, 248, 249, 249, 250, 250, 251, 251,
        252, 252, 253, 253, 254, 254, 255, 255,
      };

      /**
       * @brief Converts a squared-space linear value back to sRGB.
       * @param linearSquared A value in the range 0-65025 produced by
       *        squaring an 8-bit channel and blending.
       * @return The approximate sRGB value (0-255).
       */
      static inline UInt8 _delinearize(UInt32 linearSquared) {
        return _sqrtLUT[linearSquared >> 8];
      }
  };
}
