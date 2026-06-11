/**
 * @file Bootloader/Platform/PC/VGASpinner.cpp
 * @brief Implements @ref @QBtldr::Platform::PC::VGASpinner.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "VGASpinner.hpp"

namespace Quantum::Bootloader::Platform::PC {
  void VGASpinner::Show() {
    _visible = true;
    _frameIndex = 0;
    _lastTick = GetBIOSClockTicks();

    _put(_frames[_frameIndex]);
  }

  void VGASpinner::Tick() {
    if (!_visible) return;

    UInt32 now = GetBIOSClockTicks();

    if (now == _lastTick) return;

    _lastTick = now;
    _frameIndex = (_frameIndex + 1) & 0x03;

    _put('\b');
    _put(_frames[_frameIndex]);
  }

  void VGASpinner::Hide() {
    if (!_visible) return;

    _visible = false;
    _frameIndex = 0;
    _lastTick = 0;

    _put('\b');
    _put(' ');
    _put('\b');
  }

  void VGASpinner::_put(char c) {
    BIOSRegisters regs = {};

    regs.EAX = 0x0E00 | static_cast<UInt8>(c);

    CallBIOS(0x10, &regs);
  }
}
