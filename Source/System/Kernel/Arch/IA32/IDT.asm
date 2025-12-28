;-------------------------------------------------------------------------------
; Quantum
;-------------------------------------------------------------------------------
; File: System/Kernel/Arch/IA32/IDT.asm
; Brief: Loads and defines the Interrupt Descriptor Table (IDT).
; Author: Brandon Belna <bbelna@aol.com>
; Copyright: © 2025-2026 The Quantum OS Project
; License: GPL 2.0
;-------------------------------------------------------------------------------

[BITS 32]

global LoadIDT
global ISR0
global ISR1
global ISR2
global ISR3
global ISR4
global ISR5
global ISR6
global ISR7
global ISR8
global ISR9
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
global IRQ0
global IRQ1
global IRQ2
global IRQ3
global IRQ4
global IRQ5
global IRQ6
global IRQ7
global IRQ8
global IRQ9
global IRQ10
global IRQ11
global IRQ12
global IRQ13
global IRQ14
global IRQ15
global SYSCALL80
extern IDTExceptionHandler

; void LoadIDT(IDT::Descriptor* desc);
LoadIDT:
  mov eax, [esp + 4]  ; arg0: pointer to descriptor
  lidt [eax]
  ret

;------------------------------------------------------------------------------
; Shared ISR helpers
;------------------------------------------------------------------------------
; ISR_NOERR creates a stub for interrupts without a hardware error code.
; ISR_ERR  creates a stub for interrupts that push a hardware error code.
; Both build a consistent stack frame and pass a pointer to it to
; IDTExceptionHandler(Interrupts::Context*).
;------------------------------------------------------------------------------
%macro ISR_NOERR 2
%1:
  push dword 0          ; synthetic error code (placed below vector)
  push dword %2         ; vector number
  pusha                 ; save regs (now ESP points at EDI in context)
  push esp              ; arg0: Interrupts::Context*
  call IDTExceptionHandler
  add esp, 4            ; pop arg
  test eax, eax         ; swap to returned context if provided
  jz .%1_no_swap
  mov esp, eax
.%1_no_swap:
  popa
  add esp, 8            ; drop vector + error
  iretd
%endmacro

%macro ISR_ERR 2
%1:
  push dword %2         ; vector number (hardware error already on stack)
  pusha                 ; save regs
  push esp              ; arg0: Interrupts::Context*
  call IDTExceptionHandler
  add esp, 4            ; pop arg
  popa
  add esp, 8            ; drop vector + hardware error
  iretd
%endmacro

;------------------------------------------------------------------------------
; CPU exceptions (vectors 0-31)
;------------------------------------------------------------------------------
ISR_NOERR ISR0,  0   ; #DE Divide Error
ISR_NOERR ISR1,  1   ; #DB Debug
ISR_NOERR ISR2,  2   ; NMI Interrupt
ISR_NOERR ISR3,  3   ; #BP Breakpoint
ISR_NOERR ISR4,  4   ; #OF Overflow
ISR_NOERR ISR5,  5   ; #BR BOUND Range Exceeded
ISR_NOERR ISR6,  6   ; #UD Invalid Opcode
ISR_NOERR ISR7,  7   ; #NM Device Not Available
ISR_ERR   ISR8,  8   ; #DF Double Fault (error code)
ISR_NOERR ISR9,  9   ; Coprocessor Segment Overrun (reserved)
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

;------------------------------------------------------------------------------
; PIC interrupts (IRQs 0-15 mapped to vectors 32-47)
;------------------------------------------------------------------------------
ISR_NOERR IRQ0,  32   ; PIT
ISR_NOERR IRQ1,  33   ; Keyboard
ISR_NOERR IRQ2,  34   ; Cascade
ISR_NOERR IRQ3,  35   ; COM2
ISR_NOERR IRQ4,  36   ; COM1
ISR_NOERR IRQ5,  37   ; LPT2
ISR_NOERR IRQ6,  38   ; Floppy
ISR_NOERR IRQ7,  39   ; LPT1
ISR_NOERR IRQ8,  40   ; CMOS RTC
ISR_NOERR IRQ9,  41   ; Free/ACPI
ISR_NOERR IRQ10, 42   ; Free
ISR_NOERR IRQ11, 43   ; Free
ISR_NOERR IRQ12, 44   ; PS/2 Mouse
ISR_NOERR IRQ13, 45   ; FPU
ISR_NOERR IRQ14, 46   ; Primary ATA
ISR_NOERR IRQ15, 47   ; Secondary ATA

;------------------------------------------------------------------------------
; System call (vector 0x80)
;------------------------------------------------------------------------------
ISR_NOERR SYSCALL80, 128

.hang:
  hlt
  jmp .hang
