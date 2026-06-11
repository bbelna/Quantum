/**
 * @file Kernel/Arch/IA32/Drivers/CPU/IA32CPUDriver.hpp
 * @brief Declares @ref @QKrnlIA32::Drivers::CPU::IA32CPUDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Drivers/CPU/ICPUDriver.hpp>
#include <Drivers/DriverTypes.hpp>
#include <KernelTypes.hpp>

#include "IA32CPUVendor.hpp"

namespace Quantum::Kernel::Arch::IA32::Drivers::CPU {
  /**
   * @brief IA-32 CPU driver.
   *
   * Implements the @ref ICPUDriver interface by delegating to the IA-32
   * architecture layer. Probes CPUID at construction time to detect
   * features and hypervisor presence.
   */
  class IA32CPUDriver : public ICPUDriver {
    public:
      /**
       * @brief Constructs the CPU driver and probes CPUID.
       */
      explicit IA32CPUDriver();

      /**
       * @brief Gets the device associated with this driver.
       * @return The device associated with this driver.
       */
      Device& GetDevice() override { return _device; }

      /**
       * @brief Gets the unique identifier of the device.
       * @return The device ID.
       */
      DeviceID GetDeviceID() override { return _device.ID; }

      /**
       * @brief Invokes a CPU driver operation.
       * @param operation The operation code (see @ref CPUDriverOperation).
       * @param payload Pointer to an operation-specific payload.
       * @return Operation-specific result.
       */
      UInt32 Invoke(UInt32 operation, void* payload) override;

      const char* GetArchName() override;

      void Halt() override;

      [[noreturn]] void HaltForever() override;

      void Pause() override;

      void Yield() override;

      void DisableInterrupts() override;

      void EnableInterrupts() override;

      UInt32 SaveAndDisableInterrupts() override;

      void RestoreInterrupts(UInt32 flags) override;

      UInt8 In8(UInt16 port) override;

      UInt16 In16(UInt16 port) override;

      UInt32 In32(UInt16 port) override;

      void Out8(UInt16 port, UInt8 value) override;

      void Out16(UInt16 port, UInt16 value) override;

      void Out32(UInt16 port, UInt32 value) override;

      UInt32 Exchange32(volatile UInt32* pointer, UInt32 value) override;

      bool CompareExchange32(
        volatile UInt32* pointer,
        UInt32& expected,
        UInt32 desired
      ) override;

      bool CompareExchange64(
        volatile UInt64* pointer,
        UInt64& expected,
        UInt64 desired
      ) override;

      UInt32 FetchAdd32(volatile UInt32* pointer, UInt32 delta) override;

      void CompilerBarrier();

      void LockedBarrier();

      void LoadPageDirectory(UIntPtr pagePhysicalAddress);

      void EnablePaging();

      void EnableWriteProtect();

      void InvalidatePage(UIntPtr pageVirtualAddress);

      UInt64 ReadMSR(UInt32 msr);

      void WriteMSR(UInt32 msr, UInt64 value);

      bool HasCPUID();

      UInt8 DetectCPUGeneration();

      bool HasFPU();

      void CPUID(
        UInt32 leaf,
        UInt32& eax,
        UInt32& ebx,
        UInt32& ecx,
        UInt32& edx
      );

      void InitializePAT();

      bool IsPATSupported();

      UIntPtr VirtualToPhysical(UIntPtr virtualAddress);

      UIntPtr PhysicalToVirtual(UIntPtr physicalAddress);

      void NotifyKernelPageDirectoryLoaded();

      /**
       * @brief Returns whether a hypervisor was detected.
       */
      bool IsHypervisorPresent() const { return _hypervisorInfo.Present; }

      /**
       * @brief Returns the cached hypervisor info.
       */
      const CPUHypervisorInfo& GetHypervisorInfo() const {
        return _hypervisorInfo;
      }

    private:
      /**
       * @brief The device associated with this driver.
       */
      Device _device {
        6,
        "CPU0",
        "IA-32 Processor",
        ToDeviceCategoryID(DeviceCategoryType::CPU),
        DeviceState::Active,
        0,
        0,
        DeviceBus::None,
        0,
        {}
      };

      /**
       * @brief Whether the kernel page directory has been loaded and page
       *        table walks are safe.
       */
      bool _kernelPageDirectoryLoaded = false;

      /**
       * @brief Whether PAT (Page Attribute Table) was successfully
       *        initialized.
       */
      bool _patSupported = false;

      /**
       * @brief Detected CPU vendor.
       */
      IA32CPUVendor _vendor = IA32CPUVendor::Unknown;

      /**
       * @brief Cached hypervisor detection result.
       */
      CPUHypervisorInfo _hypervisorInfo = {};

      /**
       * @brief Cached FPU detection result.
       */
      CPUFPUInfo _fpuInfo = {};

      /**
       * @brief Probes CPUID for vendor, model, brand, and hypervisor
       *        info. Called once from the constructor.
       */
      void _probe();

      /**
       * @brief Tests whether CPUID is supported by toggling EFLAGS
       *        bit 21 (ID flag).
       * @return `true` if CPUID is available.
       */
      static bool _detectHasCPUID();

      /**
       * @brief Identifies the CPU generation via EFLAGS probing when
       *        CPUID is not available.
       * @return `3` for 386-class, `4` for 486-class, or `0` if
       *         indeterminate.
       */
      static UInt8 _detectGeneration();

      /**
       * @brief Probes for an x87 FPU by executing `FNINIT` followed by
       *        `FNSTSW` and checking whether the status word resets to
       *        zero.
       * @return `true` if an FPU responded to the probe.
       */
      static bool _detectHasFPU();

      /**
       * @brief Executes the CPUID instruction.
       * @param leaf The CPUID leaf to query.
       * @param eax Output register EAX.
       * @param ebx Output register EBX.
       * @param ecx Output register ECX.
       * @param edx Output register EDX.
       */
      static void _executeCPUID(
        UInt32 leaf,
        UInt32& eax,
        UInt32& ebx,
        UInt32& ecx,
        UInt32& edx
      );

      /**
       * @brief Initializes PAT if supported by the CPU.
       */
      void _initializePAT();

      /**
       * @brief Stores 4 bytes from a CPUID register into a character
       *        buffer.
       * @param reg The register value.
       * @param destination Pointer to at least 4 writable bytes.
       */
      static void _storeRegister(UInt32 reg, char* destination);
  };
}
