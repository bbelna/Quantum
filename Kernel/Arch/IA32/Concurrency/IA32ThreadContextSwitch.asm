;------------------------------------------------------------------------------;
; Quantum                                                                      ;
;------------------------------------------------------------------------------;
; File:      Kernel/Arch/IA32/Concurrency/IA32ThreadContextSwitch.asm       ;
; Brief:     Low-level context switch routines for IA-32.                      ;
; Author:    Brandon Belna (<bbelna@aol.com>)                                  ;
; Copyright: Copyright © 2025-2026 The Quantum Software Project.               ;
;            All rights reserved.                                              ;
;            Licensed under the GNU General Public License v2.0-only.          ;
;            See LICENSE.md for details.                                       ;
;------------------------------------------------------------------------------;

[BITS 32]

section .text

; ------------------------------------------------------------------------------
; StartThreading
; ------------------------------------------------------------------------------
; [[noreturn]] void StartThreading(InterruptContext* context)
; ------------------------------------------------------------------------------
; Brief: Bootstraps into the first thread by loading its InterruptContext
;        and performing an iret. Used to start the scheduler.
; Inputs:
;   - [esp+4]: Pointer to the InterruptContext to load.
;
; Loads the given InterruptContext and performs an iret to start
; executing the thread. Used to enter the scheduler for the first time.
; ------------------------------------------------------------------------------
global StartThreading
StartThreading:
  ; get pointer to InterruptContext
  mov esp, [esp + 4]

  ; restore segment registers
  pop gs
  pop fs
  pop es
  pop ds

  ; restore general purpose registers (pusha order, reversed)
  pop edi
  pop esi
  pop ebp
  add esp, 4        ; skip ESP in pusha (we're using our own stack)
  pop ebx
  pop edx
  pop ecx
  pop eax

  ; skip vector and error code
  add esp, 8

  ; iret will pop EIP, CS, EFLAGS (and for user mode: ESP, SS)
  iret

SECTION .note.GNU-stack
