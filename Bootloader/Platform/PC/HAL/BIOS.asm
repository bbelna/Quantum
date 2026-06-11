;------------------------------------------------------------------------------;
; Quantum Bootloader                                                           ;
;------------------------------------------------------------------------------;
; File:      Bootloader/Platform/PC/HAL/BIOS.asm                                          ;
; Brief:     BIOS trampoline for calling real-mode BIOS interrupts from        ;
;            32-bit protected mode.                                            ;
; Author:    Brandon Belna (<bbelna@aol.com>)                                    ;
; Copyright: Copyright © 2025-2026 The Quantum Software Project.                     ;
;            All rights reserved.                                              ;
;            Licensed under the GNU General Public License v2.0-only.          ;
;            See LICENSE.md for details.                                       ;
;------------------------------------------------------------------------------;

[BITS 32]

; --- Selectors (must match GDT order below) ---
Code32Selector equ 0x08
Data32Selector equ 0x10
Code16Selector equ 0x18
Data16Selector equ 0x20

; --- Real-mode trampoline stack ---
; Must be below the boot binary (0x5000) and above the FAT buffer end (~0x4200).
RealModeStack  equ 0x4C00

; ------------------------------------------------------------------------------
; Data
; ------------------------------------------------------------------------------

section .data

align 8

BootGDT:
  dq 0x0000000000000000       ; 0x00 - null
  dq 0x00CF9A000000FFFF       ; 0x08 - 32-bit code (base=0, limit=4GB, RX)
  dq 0x00CF92000000FFFF       ; 0x10 - 32-bit data (base=0, limit=4GB, RW)
  dq 0x00009A000000FFFF       ; 0x18 - 16-bit code (base=0, limit=64KB, RX)
  dq 0x008F92000000FFFF       ; 0x20 - 16-bit data (base=0, limit=4GB, RW)

BootGDTDescriptor:
  dw 5 * 8 - 1                ; limit = 40 - 1 = 39
  dd BootGDT                   ; base

RealModeIDTR:
  dw 0x03FF                    ; limit: 256 entries * 4 bytes
  dd 0x00000000                ; base: 0

SavedESP:            dd 0
SavedEBP:            dd 0
BIOSRegsPtr:         dd 0
SavedFlags:          dw 0
SavedIDTRDescriptor: dw 0
                     dd 0

; ------------------------------------------------------------------------------
; Text
; ------------------------------------------------------------------------------

section .text

; ------------------------------------------------------------------------------
; InitializeBIOS
; ------------------------------------------------------------------------------
; Brief: Loads Boot's GDT. Call once from Main before any CallBIOS use.
; ------------------------------------------------------------------------------
global InitializeBIOS

InitializeBIOS:
  lgdt [BootGDTDescriptor]
  ret

; ------------------------------------------------------------------------------
; CallBIOS
; ------------------------------------------------------------------------------
; Brief: Calls a real-mode BIOS interrupt from 32-bit protected mode.
;
; C prototype: extern "C" void CallBIOS(UInt8 interruptNumber,
;                                        BIOSRegisters* regs);
;
; Uses cdecl calling convention:
;   [ebp + 8]  = interruptNumber (UInt8, passed as dword)
;   [ebp + 12] = regs (BIOSRegisters*)
;
; BIOSRegisters struct layout:
;   +0   EAX     +4   EBX     +8   ECX     +12  EDX
;   +16  ESI     +20  EDI     +24  DS      +26  ES
;   +28  EFLAGS (output only)
; ------------------------------------------------------------------------------
global CallBIOS

CallBIOS:
  push ebp
  mov ebp, esp
  pushad
  pushfd

  ; save 32-bit stack and IDTR
  mov [SavedESP], esp
  mov [SavedEBP], ebp
  sidt [SavedIDTRDescriptor]

  ; get parameters
  mov al, [ebp + 8]           ; interrupt number
  mov ebx, [ebp + 12]         ; BIOSRegisters*
  mov [BIOSRegsPtr], ebx

  ; patch the INT instruction with the actual interrupt number
  mov [.IntOpcode + 1], al

  ; PM32 -> PM16
  jmp Code16Selector:.Trampoline16

[BITS 16]

.Trampoline16:
  ; load 16-bit data segments (4GB limit for "unreal" data access)
  mov ax, Data16Selector
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  ; PM16 -> real mode
  mov eax, cr0
  and eax, 0xFFFFFFFE         ; clear PE bit
  mov cr0, eax

  ; far jump to flush pipeline into real mode (CS=0)
  jmp 0x0000:.RealMode

.RealMode:
  ; set up real-mode segments
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  mov sp, RealModeStack

  ; load real-mode IDT
  lidt [RealModeIDTR]

  ; load registers from BIOSRegisters
  ; (DS=0, so we use absolute addressing via [BIOSRegsPtr])
  mov ebx, [BIOSRegsPtr]

  ; load ES first (before we potentially change DS)
  mov ax, [ebx + 26]
  mov es, ax

  ; load general-purpose registers
  mov ecx, [ebx + 8]
  mov edx, [ebx + 12]
  mov esi, [ebx + 16]
  mov edi, [ebx + 20]

  ; save DS value and EBX/EAX for last (need DS=0 and EBX=ptr to read struct)
  push word [ebx + 24]        ; push DS value from struct
  mov eax, [ebx + 0]          ; EAX from struct
  mov ebx, [ebx + 4]          ; EBX from struct (clobbers pointer)
  pop ds                      ; DS from struct

  ; execute BIOS interrupt
  sti

.IntOpcode:
  int 0x00                     ; second byte is patched with actual int number

  cli

  ; capture BIOS return FLAGS immediately (before any flag-clobbering ops)
  pushf
  pop word [SavedFlags]

  ; save output registers back to struct
  ; need DS=0 to access our data, so save DS first
  push ds
  push eax
  push ebx

  xor ax, ax
  mov ds, ax

  mov ebx, [BIOSRegsPtr]

  ; save registers we pushed
  pop dword [ebx + 4]         ; EBX output
  pop dword [ebx + 0]         ; EAX output
  pop word [ebx + 24]         ; DS output

  ; save the rest
  mov [ebx + 8], ecx
  mov [ebx + 12], edx
  mov [ebx + 16], esi
  mov [ebx + 20], edi

  mov ax, es
  mov [ebx + 26], ax

  ; save FLAGS returned by the BIOS interrupt (captured above)
  mov ax, [SavedFlags]
  movzx eax, ax
  mov [ebx + 28], eax

  ; real mode -> PM
  ; ensure all data segments are 0 before re-entering PM
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  lgdt [BootGDTDescriptor]

  mov eax, cr0
  or eax, 1                   ; set PE bit
  mov cr0, eax

  ; far jump to 32-bit code segment
  jmp Code32Selector:.Back32

[BITS 32]

.Back32:
  ; reload 32-bit data segments
  mov ax, Data32Selector
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  ; restore 32-bit stack and IDTR
  mov esp, [SavedESP]
  mov ebp, [SavedEBP]
  lidt [SavedIDTRDescriptor]

  ; restore state
  popfd
  popad
  pop ebp
  ret
