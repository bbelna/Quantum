/**
 * @file Include/Quantum/Core/CRC32.hpp
 * @brief Freestanding CRC32 implementation (ISO 3309 / ITU-T V.42).
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Core {
  /**
   * @brief CRC32 lookup table (polynomial 0xEDB88320, reflected).
   *
   * Generated at compile time. Each entry `Table[i]` is the CRC32 of the
   * single byte `i`.
   */
  namespace CRC32Detail {
    /**
     * @brief Computes one CRC32 table entry at compile time.
     * @param index The byte value (0..255).
     * @return The CRC32 remainder for that byte.
     */
    inline constexpr UInt32 ComputeTableEntry(UInt32 index) {
      UInt32 remainder = index;

      for (UInt32 bit = 0; bit < 8; ++bit) {
        if (remainder & 1) {
          remainder = (remainder >> 1) ^ 0xEDB88320;
        } else {
          remainder >>= 1;
        }
      }

      return remainder;
    }

    /**
     * @brief 256-entry CRC32 lookup table, initialized at compile time.
     */
    inline constexpr UInt32 Table[256] = {
      ComputeTableEntry(0),   ComputeTableEntry(1),
      ComputeTableEntry(2),   ComputeTableEntry(3),
      ComputeTableEntry(4),   ComputeTableEntry(5),
      ComputeTableEntry(6),   ComputeTableEntry(7),
      ComputeTableEntry(8),   ComputeTableEntry(9),
      ComputeTableEntry(10),  ComputeTableEntry(11),
      ComputeTableEntry(12),  ComputeTableEntry(13),
      ComputeTableEntry(14),  ComputeTableEntry(15),
      ComputeTableEntry(16),  ComputeTableEntry(17),
      ComputeTableEntry(18),  ComputeTableEntry(19),
      ComputeTableEntry(20),  ComputeTableEntry(21),
      ComputeTableEntry(22),  ComputeTableEntry(23),
      ComputeTableEntry(24),  ComputeTableEntry(25),
      ComputeTableEntry(26),  ComputeTableEntry(27),
      ComputeTableEntry(28),  ComputeTableEntry(29),
      ComputeTableEntry(30),  ComputeTableEntry(31),
      ComputeTableEntry(32),  ComputeTableEntry(33),
      ComputeTableEntry(34),  ComputeTableEntry(35),
      ComputeTableEntry(36),  ComputeTableEntry(37),
      ComputeTableEntry(38),  ComputeTableEntry(39),
      ComputeTableEntry(40),  ComputeTableEntry(41),
      ComputeTableEntry(42),  ComputeTableEntry(43),
      ComputeTableEntry(44),  ComputeTableEntry(45),
      ComputeTableEntry(46),  ComputeTableEntry(47),
      ComputeTableEntry(48),  ComputeTableEntry(49),
      ComputeTableEntry(50),  ComputeTableEntry(51),
      ComputeTableEntry(52),  ComputeTableEntry(53),
      ComputeTableEntry(54),  ComputeTableEntry(55),
      ComputeTableEntry(56),  ComputeTableEntry(57),
      ComputeTableEntry(58),  ComputeTableEntry(59),
      ComputeTableEntry(60),  ComputeTableEntry(61),
      ComputeTableEntry(62),  ComputeTableEntry(63),
      ComputeTableEntry(64),  ComputeTableEntry(65),
      ComputeTableEntry(66),  ComputeTableEntry(67),
      ComputeTableEntry(68),  ComputeTableEntry(69),
      ComputeTableEntry(70),  ComputeTableEntry(71),
      ComputeTableEntry(72),  ComputeTableEntry(73),
      ComputeTableEntry(74),  ComputeTableEntry(75),
      ComputeTableEntry(76),  ComputeTableEntry(77),
      ComputeTableEntry(78),  ComputeTableEntry(79),
      ComputeTableEntry(80),  ComputeTableEntry(81),
      ComputeTableEntry(82),  ComputeTableEntry(83),
      ComputeTableEntry(84),  ComputeTableEntry(85),
      ComputeTableEntry(86),  ComputeTableEntry(87),
      ComputeTableEntry(88),  ComputeTableEntry(89),
      ComputeTableEntry(90),  ComputeTableEntry(91),
      ComputeTableEntry(92),  ComputeTableEntry(93),
      ComputeTableEntry(94),  ComputeTableEntry(95),
      ComputeTableEntry(96),  ComputeTableEntry(97),
      ComputeTableEntry(98),  ComputeTableEntry(99),
      ComputeTableEntry(100), ComputeTableEntry(101),
      ComputeTableEntry(102), ComputeTableEntry(103),
      ComputeTableEntry(104), ComputeTableEntry(105),
      ComputeTableEntry(106), ComputeTableEntry(107),
      ComputeTableEntry(108), ComputeTableEntry(109),
      ComputeTableEntry(110), ComputeTableEntry(111),
      ComputeTableEntry(112), ComputeTableEntry(113),
      ComputeTableEntry(114), ComputeTableEntry(115),
      ComputeTableEntry(116), ComputeTableEntry(117),
      ComputeTableEntry(118), ComputeTableEntry(119),
      ComputeTableEntry(120), ComputeTableEntry(121),
      ComputeTableEntry(122), ComputeTableEntry(123),
      ComputeTableEntry(124), ComputeTableEntry(125),
      ComputeTableEntry(126), ComputeTableEntry(127),
      ComputeTableEntry(128), ComputeTableEntry(129),
      ComputeTableEntry(130), ComputeTableEntry(131),
      ComputeTableEntry(132), ComputeTableEntry(133),
      ComputeTableEntry(134), ComputeTableEntry(135),
      ComputeTableEntry(136), ComputeTableEntry(137),
      ComputeTableEntry(138), ComputeTableEntry(139),
      ComputeTableEntry(140), ComputeTableEntry(141),
      ComputeTableEntry(142), ComputeTableEntry(143),
      ComputeTableEntry(144), ComputeTableEntry(145),
      ComputeTableEntry(146), ComputeTableEntry(147),
      ComputeTableEntry(148), ComputeTableEntry(149),
      ComputeTableEntry(150), ComputeTableEntry(151),
      ComputeTableEntry(152), ComputeTableEntry(153),
      ComputeTableEntry(154), ComputeTableEntry(155),
      ComputeTableEntry(156), ComputeTableEntry(157),
      ComputeTableEntry(158), ComputeTableEntry(159),
      ComputeTableEntry(160), ComputeTableEntry(161),
      ComputeTableEntry(162), ComputeTableEntry(163),
      ComputeTableEntry(164), ComputeTableEntry(165),
      ComputeTableEntry(166), ComputeTableEntry(167),
      ComputeTableEntry(168), ComputeTableEntry(169),
      ComputeTableEntry(170), ComputeTableEntry(171),
      ComputeTableEntry(172), ComputeTableEntry(173),
      ComputeTableEntry(174), ComputeTableEntry(175),
      ComputeTableEntry(176), ComputeTableEntry(177),
      ComputeTableEntry(178), ComputeTableEntry(179),
      ComputeTableEntry(180), ComputeTableEntry(181),
      ComputeTableEntry(182), ComputeTableEntry(183),
      ComputeTableEntry(184), ComputeTableEntry(185),
      ComputeTableEntry(186), ComputeTableEntry(187),
      ComputeTableEntry(188), ComputeTableEntry(189),
      ComputeTableEntry(190), ComputeTableEntry(191),
      ComputeTableEntry(192), ComputeTableEntry(193),
      ComputeTableEntry(194), ComputeTableEntry(195),
      ComputeTableEntry(196), ComputeTableEntry(197),
      ComputeTableEntry(198), ComputeTableEntry(199),
      ComputeTableEntry(200), ComputeTableEntry(201),
      ComputeTableEntry(202), ComputeTableEntry(203),
      ComputeTableEntry(204), ComputeTableEntry(205),
      ComputeTableEntry(206), ComputeTableEntry(207),
      ComputeTableEntry(208), ComputeTableEntry(209),
      ComputeTableEntry(210), ComputeTableEntry(211),
      ComputeTableEntry(212), ComputeTableEntry(213),
      ComputeTableEntry(214), ComputeTableEntry(215),
      ComputeTableEntry(216), ComputeTableEntry(217),
      ComputeTableEntry(218), ComputeTableEntry(219),
      ComputeTableEntry(220), ComputeTableEntry(221),
      ComputeTableEntry(222), ComputeTableEntry(223),
      ComputeTableEntry(224), ComputeTableEntry(225),
      ComputeTableEntry(226), ComputeTableEntry(227),
      ComputeTableEntry(228), ComputeTableEntry(229),
      ComputeTableEntry(230), ComputeTableEntry(231),
      ComputeTableEntry(232), ComputeTableEntry(233),
      ComputeTableEntry(234), ComputeTableEntry(235),
      ComputeTableEntry(236), ComputeTableEntry(237),
      ComputeTableEntry(238), ComputeTableEntry(239),
      ComputeTableEntry(240), ComputeTableEntry(241),
      ComputeTableEntry(242), ComputeTableEntry(243),
      ComputeTableEntry(244), ComputeTableEntry(245),
      ComputeTableEntry(246), ComputeTableEntry(247),
      ComputeTableEntry(248), ComputeTableEntry(249),
      ComputeTableEntry(250), ComputeTableEntry(251),
      ComputeTableEntry(252), ComputeTableEntry(253),
      ComputeTableEntry(254), ComputeTableEntry(255),
    };
  }

  /**
   * @brief Computes the CRC32 of a byte buffer.
   * @param data Pointer to the data buffer.
   * @param sizeInBytes Number of bytes to process.
   * @param initialValue Initial CRC value (`0xFFFFFFFF` for a new
   *                     computation, or a previous result for chained
   *                     computation).
   * @return The CRC32 value. For a final result, XOR with `0xFFFFFFFF`.
   */
  inline UInt32 CRC32Update(
    const void* data,
    Size sizeInBytes,
    UInt32 initialValue = 0xFFFFFFFF
  ) {
    auto* bytes = static_cast<const UInt8*>(data);
    UInt32 crc = initialValue;

    for (Size i = 0; i < sizeInBytes; ++i) {
      UInt8 index = static_cast<UInt8>(crc ^ bytes[i]);

      crc = (crc >> 8) ^ CRC32Detail::Table[index];
    }

    return crc;
  }

  /**
   * @brief Computes the final CRC32 of a byte buffer.
   * @param data Pointer to the data buffer.
   * @param sizeInBytes Number of bytes to process.
   * @return The finalized CRC32 value.
   */
  inline UInt32 CRC32(const void* data, Size sizeInBytes) {
    return CRC32Update(data, sizeInBytes) ^ 0xFFFFFFFF;
  }
}
