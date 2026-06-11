;------------------------------------------------------------------------------;
; Quantum                                                                      ;
;------------------------------------------------------------------------------;
; File:      Kernel/Arch/IA32/UserMode/IA32EnterUserMode.asm                ;
; Brief:     IA-32 user mode entry assembly routine.                           ;
; Author:    Brandon Belna (<bbelna@aol.com>)                                  ;
; Copyright: Copyright © 2025-2026 The Quantum Software Project.               ;
;            All rights reserved.                                              ;
;            Licensed under the GNU General Public License v2.0-only.          ;
;            See LICENSE.md for details.                                       ;
;------------------------------------------------------------------------------;

[BITS 32]

global EnterUserMode

;-------------------------------------------------------------------------------
; EnterUserMode
;-------------------------------------------------------------------------------
; extern "C" [[noreturn]] void EnterUserMode(
;   UInt32 userCS,    // [ebp+8]
;   UInt32 userSS,    // [ebp+12]
;   UInt32 userEIP,   // [ebp+16]
;   UInt32 userESP,   // [ebp+20]
;   UInt32 eflags     // [ebp+24]
; );
; ------------------------------------------------------------------------------
; Brief: Transitions to user mode (ring 3) via iret.
; Inputs:
;   - [ebp+8]:   User code segment selector.
;   - [ebp+12]:  User stack segment selector.
;   - [ebp+16]:  User instruction pointer.
;   - [ebp+20]:  User stack pointer.
;   - [ebp+24]:  EFLAGS to set in user mode.
;-------------------------------------------------------------------------------
EnterUserMode:
  push ebp
  mov ebp, esp

  ; load user data segment into DS, ES, FS, GS before iret
  ; the user stack segment (userSS) is also the user data segment
  mov eax, [ebp + 12]   ; userSS
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  ; build the iret stack frame (bottom to top): SS, ESP, EFLAGS, CS, EIP
  push dword [ebp + 12] ; SS     - User stack segment
  push dword [ebp + 20] ; ESP    - User stack pointer
  push dword [ebp + 24] ; EFLAGS
  push dword [ebp + 8]  ; CS     - User code segment
  push dword [ebp + 16] ; EIP    - User entry point

  ; transition to user mode
  iret

SECTION .note.GNU-stack noalloc noexec nowrite
