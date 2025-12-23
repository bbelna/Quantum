/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Floppy.hpp
 * IA32 floppy controller interrupt handling.
 */

#pragma once

#include "Devices/BlockDevice.hpp"
#include "Interrupts.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 floppy controller interrupt handler.
   */
  class Floppy {
    public:
      /**
       * Initializes the floppy IRQ handler.
       */
      static void Initialize();

    private:
      /**
       * Floppy drive index for drive A.
       */
      static constexpr UInt8 _driveAIndex = 0;

      /**
       * Floppy drive index for drive B.
       */
      static constexpr UInt8 _driveBIndex = 1;

      /**
       * CMOS register that reports floppy drive types.
       */
      static constexpr UInt8 _cmosDriveTypeRegister = 0x10;

      /**
       * CMOS address port.
       */
      static constexpr UInt16 _cmosAddressPort = 0x70;

      /**
       * CMOS data port.
       */
      static constexpr UInt16 _cmosDataPort = 0x71;

      /**
       * Boot drive magic value encoded in `BootInfo::reserved`.
       */
      static constexpr UInt32 _bootDriveMagic = 0x424F0000;

      /**
       * Default sector count used for fallback floppy registration.
       */
      static constexpr UInt32 _defaultSectorCount = 80 * 2 * 18;

      /**
       * Registered floppy device descriptors.
       */
      static Devices::BlockDevice::Device _devices[2];

      /**
       * IRQ6 handler for floppy controller interrupts.
       * @param context
       *   Interrupt context.
       * @return
       *   Updated interrupt context.
       */
      static Interrupts::Context* IRQHandler(
        Interrupts::Context& context
      );

      /**
       * Reads a CMOS register.
       * @param index
       *   CMOS register index.
       * @return
       *   Register value.
       */
      static UInt8 ReadCMOSRegister(UInt8 index);

      /**
       * Determines sector count from CMOS drive type.
       * @param driveType
       *   CMOS drive type value.
       * @param sectorCount
       *   Receives the total sector count.
       * @return
       *   True if the drive type is recognized.
       */
      static bool TryGetSectorCount(
        UInt8 driveType,
        UInt32& sectorCount
      );

      /**
       * Extracts the drive type from CMOS bits.
       * @param driveTypes
       *   CMOS drive type byte.
       * @param driveIndex
       *   Drive index to extract.
       * @return
       *   Drive type value, or 0 if invalid.
       */
      static UInt8 GetDriveType(UInt8 driveTypes, UInt8 driveIndex);

      /**
       * Detects a floppy drive and derives its sector count.
       * @param driveTypes
       *   CMOS drive type byte.
       * @param driveIndex
       *   Drive index to detect.
       * @param driveType
       *   Receives the detected drive type.
       * @param sectorCount
       *   Receives the sector count.
       * @return
       *   True if the drive is present and supported.
       */
      static bool DetectDrive(
        UInt8 driveTypes,
        UInt8 driveIndex,
        UInt8& driveType,
        UInt32& sectorCount
      );

      /**
       * Reads boot drive info from BootInfo.
       * @param bootDrive
       *   Receives the boot drive index.
       * @return
       *   True if boot info is valid.
       */
      static bool GetBootDrive(UInt8& bootDrive);
  };
}
