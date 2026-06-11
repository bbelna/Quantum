;------------------------------------------------------------------------------;
; Quantum                                                                      ;
;------------------------------------------------------------------------------;
; File:      Kernel/Arch/IA32/Interrupts/IA32IDT.asm   ;
; Brief:     IA-32 interrupt descriptor table (IDT) setup and handlers.        ;
; Author:    Brandon Belna (<bbelna@aol.com>)                                  ;
; Copyright: Copyright © 2025-2026 The Quantum Software Project.               ;
;            All rights reserved.                                              ;
;            Licensed under the GNU General Public License v2.0-only.          ;
;            See LICENSE.md for details.                                       ;
;------------------------------------------------------------------------------;

[BITS 32]

;-------------------------------------------------------------------------------
; Handlers
;-------------------------------------------------------------------------------

global LoadIDT

global ISR00
global ISR01
global ISR02
global ISR03
global ISR04
global ISR05
global ISR06
global ISR07
global ISR08
global ISR09
global ISR10
global ISR11
global ISR12
global ISR13
global ISR14
global ISR15
global ISR16
global ISR17
global ISR18
global ISR19
global ISR20
global ISR21
global ISR22
global ISR23
global ISR24
global ISR25
global ISR26
global ISR27
global ISR28
global ISR29
global ISR30
global ISR31

global IRQ00
global IRQ01
global IRQ02
global IRQ03
global IRQ04
global IRQ05
global IRQ06
global IRQ07
global IRQ08
global IRQ09
global IRQ10
global IRQ11
global IRQ12
global IRQ13
global IRQ14
global IRQ15

global YIELD49

global SYSCALL80
global SYSENTER

extern IA32IDTExceptionHandler

; ------------------------------------------------------------------------------
; LoadIDT
; ------------------------------------------------------------------------------
; void LoadIDT(Descriptor* desc);
; ------------------------------------------------------------------------------
; Brief: Loads the Interrupt Descriptor Table (IDT) from the given descriptor
;        pointer using the lidt instruction.
; Inputs:
;   - [esp+4]: Pointer to the IDT descriptor.
; ------------------------------------------------------------------------------
LoadIDT:
  mov eax, [esp + 4]  ; arg0: pointer to descriptor
  lidt [eax]
  ret

; ------------------------------------------------------------------------------
; ISR_NOERR
; ------------------------------------------------------------------------------
; Brief: Macro to define an interrupt service routine (ISR) for interrupts
;        that do not push an error code.
; Inputs:
;   - %1: Label name for the ISR.
;   - %2: Interrupt vector number.
; ------------------------------------------------------------------------------
%macro ISR_NOERR 2
%1:
  push dword 0          ; synthetic error code
  push dword %2         ; vector number
  pusha                 ; save GPRs
  push ds               ; save segment registers
  push es
  push fs
  push gs
  mov ax, 0x10          ; load kernel data segment
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  push esp              ; arg0: InterruptContext*
  call IA32IDTExceptionHandler
  add esp, 4            ; pop arg
  test eax, eax         ; swap to returned context if provided
  jz .%1_no_swap
  mov esp, eax

  .%1_no_swap:
    pop gs              ; restore segment registers
    pop fs
    pop es
    pop ds
    popa                ; restore GPRs
    add esp, 8          ; drop vector + error
    iretd
%endmacro

; ------------------------------------------------------------------------------
; ISR_ERR
; ------------------------------------------------------------------------------
; Brief: Macro to define an interrupt service routine (ISR) for interrupts
;        that push an error code.
; Inputs:
;   - %1: Label name for the ISR.
;   - %2: Interrupt vector number.
; ------------------------------------------------------------------------------
%macro ISR_ERR 2
%1:
  push dword %2         ; vector number (hardware error already on stack)
  pusha                 ; save GPRs
  push ds               ; save segment registers
  push es
  push fs
  push gs
  mov ax, 0x10          ; load kernel data segment
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  push esp              ; arg0: InterruptContext*
  call IA32IDTExceptionHandler
  add esp, 4            ; pop arg
  test eax, eax
  jz .%1_no_swap
  mov esp, eax

  .%1_no_swap:
    pop gs              ; restore segment registers
    pop fs
    pop es
    pop ds
    popa                ; restore GPRs
    add esp, 8          ; drop vector + hardware error
    iretd
%endmacro

;-------------------------------------------------------------------------------
; CPU exceptions (vectors 0-31)
;-------------------------------------------------------------------------------

ISR_NOERR ISR00,  0  ; #DE Divide Error
ISR_NOERR ISR01,  1  ; #DB Debug
ISR_NOERR ISR02,  2  ; NMI Interrupt
ISR_NOERR ISR03,  3  ; #BP Breakpoint
ISR_NOERR ISR04,  4  ; #OF Overflow
ISR_NOERR ISR05,  5  ; #BR BOUND Range Exceeded
ISR_NOERR ISR06,  6  ; #UD Invalid Opcode
ISR_NOERR ISR07,  7  ; #NM Device Not Available
ISR_ERR   ISR08,  8  ; #DF Double Fault (error code)
ISR_NOERR ISR09,  9  ; Coprocessor Segment Overrun (reserved)
ISR_ERR   ISR10, 10  ; #TS Invalid TSS (error code)
ISR_ERR   ISR11, 11  ; #NP Segment Not Present (error code)
ISR_ERR   ISR12, 12  ; #SS Stack-Segment Fault (error code)
ISR_ERR   ISR13, 13  ; #GP General Protection (error code)
ISR_ERR   ISR14, 14  ; #PF Page Fault (error code)
ISR_NOERR ISR15, 15  ; Intel reserved
ISR_NOERR ISR16, 16  ; #MF x87 FPU Floating-Point Error
ISR_ERR   ISR17, 17  ; #AC Alignment Check (error code)
ISR_NOERR ISR18, 18  ; #MC Machine Check
ISR_NOERR ISR19, 19  ; #XM SIMD Floating-Point Exception
ISR_NOERR ISR20, 20  ; #VE Virtualization Exception
ISR_NOERR ISR21, 21  ; Intel reserved
ISR_NOERR ISR22, 22  ; Intel reserved
ISR_NOERR ISR23, 23  ; Intel reserved
ISR_NOERR ISR24, 24  ; Intel reserved
ISR_NOERR ISR25, 25  ; Intel reserved
ISR_NOERR ISR26, 26  ; Intel reserved
ISR_NOERR ISR27, 27  ; Intel reserved
ISR_NOERR ISR28, 28  ; Intel reserved
ISR_NOERR ISR29, 29  ; Intel reserved
ISR_NOERR ISR30, 30  ; #SX Security Exception
ISR_NOERR ISR31, 31  ; Intel reserved

;-------------------------------------------------------------------------------
; PIC interrupts (IRQs 0-15 mapped to vectors 32-47)
;-------------------------------------------------------------------------------

ISR_NOERR IRQ00, 32  ; PIT
ISR_NOERR IRQ01, 33  ; Keyboard
ISR_NOERR IRQ02, 34  ; Cascade
ISR_NOERR IRQ03, 35  ; COM2
ISR_NOERR IRQ04, 36  ; COM1
ISR_NOERR IRQ05, 37  ; LPT2
ISR_NOERR IRQ06, 38  ; Floppy
ISR_NOERR IRQ07, 39  ; LPT1
ISR_NOERR IRQ08, 40  ; CMOS RTC
ISR_NOERR IRQ09, 41  ; Free/ACPI
ISR_NOERR IRQ10, 42  ; Free
ISR_NOERR IRQ11, 43  ; Free
ISR_NOERR IRQ12, 44  ; PS/2 Mouse
ISR_NOERR IRQ13, 45  ; FPU
ISR_NOERR IRQ14, 46  ; Primary ATA
ISR_NOERR IRQ15, 47  ; Secondary ATA

;------------------------------------------------------------------------------
; Software yield (vector 49)
;------------------------------------------------------------------------------
ISR_NOERR YIELD49, 49

;------------------------------------------------------------------------------
; System call (vector 0x80)
;------------------------------------------------------------------------------
ISR_NOERR SYSCALL80, 128

;------------------------------------------------------------------------------
; SYSENTER entry point
;------------------------------------------------------------------------------
; Fast system call entry via SYSENTER instruction.
;
; User-space register convention:
;   EAX = operation    EBX = arg1
;   ESI = arg2         EDI = arg3
;   ECX = user ESP     EDX = user return EIP
;
; Builds an InterruptContext + iret frame identical to the int 0x80 path so
; that SystemCallHandler and context switching work unchanged. The fast-path
; exit uses SYSEXIT; context switches fall back to IRETD.
;------------------------------------------------------------------------------
SYSENTER:
  cli                          ; sysenter does NOT clear IF; match int gate

  ; build iret frame (needed if this thread is later resumed via iretd)
  push dword 0x23              ; SS_user
  push ecx                     ; ESP_user
  pushfd                       ; EFLAGS (IF=0 after cli)
  or dword [esp], 0x200        ; set IF so iretd resumes with interrupts on
  push dword 0x1B              ; CS_user
  push edx                     ; EIP_user (sysenter return address)

  ; InterruptContext header
  push dword 0                 ; synthetic error code
  push dword 128               ; vector (same as int 0x80)

  ; remap user args to match int $0x80 register convention
  ; handler reads ECX for arg2, EDX for arg3
  mov ecx, esi
  mov edx, edi

  pusha                        ; save GPRs (includes user ESI/EDI in their
                               ; canonical positions)
  push ds                      ; save segment registers
  push es
  push fs
  push gs
  mov ax, 0x10                 ; load kernel data segment
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  push esp                     ; arg0: InterruptContext*
  call IA32IDTExceptionHandler
  add esp, 4

  test eax, eax                ; context switch requested?
  jnz .sysenter_switch

  ; fast path: return to same thread via sysexit
  ;
  ; Multi-return syscalls write results to context.EDI / context.ESI
  ; (the canonical pusha slots), so popa restores the correct values
  ; into ESI/EDI directly.  No ECX/EDX shuttle needed.

  pop gs
  pop fs
  pop es
  pop ds
  popa                         ; restores all GPRs including ESI/EDI
  add esp, 8                   ; skip vector + error code

  ; load sysexit operands from iret frame
  mov edx, [esp]               ; EIP_user
  mov ecx, [esp + 12]          ; ESP_user (skip CS + EFLAGS)

  sti                          ; sti + next insn is atomic on Intel
  sysexit

.sysenter_switch:
  ; slow path: context switch via iretd - the InterruptContext's
  ; ESI/EDI slots hold the user's original values (pusha saved them
  ; before the handler ran), so popa + iretd restores them correctly.
  mov esp, eax
  pop gs
  pop fs
  pop es
  pop ds
  popa
  add esp, 8

  iretd

.Hang:
  hlt
  jmp .Hang

SECTION .note.GNU-stack
