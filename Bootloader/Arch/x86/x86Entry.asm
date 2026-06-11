;------------------------------------------------------------------------------;
; Quantum Bootloader                                                           ;
;------------------------------------------------------------------------------;
; File:      Bootloader/Arch/x86/x86Entry.asm                               ;
; Brief:     x86 entry point for the bootloader.                               ;
; Author:    Brandon Belna (<bbelna@aol.com>)                                  ;
; Copyright: Copyright © 2025-2026 The Quantum Software Project.               ;
;            All rights reserved.                                              ;
;            Licensed under the GNU General Public License v2.0-only.          ;
;            See LICENSE.md for details.                                       ;
;------------------------------------------------------------------------------;

[BITS 32]

extern Main
extern __bss_start
extern __bss_end

section .text.start

global BootEntry

; ------------------------------------------------------------------------------
; BootEntry
; ------------------------------------------------------------------------------
; Brief: Entry point called from stage 2's ProtectedEntry.
; Inputs:
;   - ESI: Physical pointer to the BootInfo structure.
; ------------------------------------------------------------------------------
BootEntry:
  ; zero BSS
  mov edi, __bss_start
  mov ecx, __bss_end
  sub ecx, edi
  shr ecx, 2             ; byte count -> dword count
  xor eax, eax
  rep stosd

  ; pass BootInfo address as argument to Main
  push esi
  call Main

  ; should never return
  cli

  .Hang:
    hlt
    jmp .Hang
