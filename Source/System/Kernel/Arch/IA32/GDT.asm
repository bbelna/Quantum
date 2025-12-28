;-------------------------------------------------------------------------------
; Quantum
;-------------------------------------------------------------------------------
; File: System/Kernel/Arch/IA32/GDT.asm
; Brief: Defines a minimal GDT and its descriptor for 32-bit mode.
; Author: Brandon Belna <bbelna@aol.com>
; Copyright: © 2025-2026 The Quantum OS Project
; License: GPL 2.0
;-------------------------------------------------------------------------------

BITS 32

global gdt
global gdtDescriptor32

SECTION .text.start
gdt:
  ; Null descriptor (all zeros)
  dq 0x0000000000000000

  ; Code segment: base=0, limit=0xFFFFF, type=0x9A, flags=0xCF
  ; 0x00CF9A000000FFFF means:
  ;  - limit_low = 0xFFFF
  ;  - base_low = 0x0000
  ;  - base_mid = 0x00
  ;  - access = 0x9A   (present, ring0, code segment, executable, readable)
  ;  - flags = 0xC    (4K granularity, 32-bit, long=0)
  ;  - limit_high = 0xF
  ;  - base_high = 0x00
  dq 0x00CF9A000000FFFF

  ; Data segment: base=0, limit=0xFFFFF, type=0x92, flags=0xCF
  ; 0x00CF92000000FFFF means:
  ;  - limit_low = 0xFFFF
  ;  - base_low = 0x0000
  ;  - base_mid = 0x00
  ;  - access = 0x92   (present, ring0, data segment, writable)
  ;  - flags = 0xC    (4K granularity, 32-bit)
  ;  - limit_high = 0xF
  ;  - base_high = 0x00
  dq 0x00CF92000000FFFF

  ; User code segment: base=0, limit=0xFFFFF, type=0xFA, flags=0xCF
  dq 0x00CFFA000000FFFF

  ; User data segment: base=0, limit=0xFFFFF, type=0xF2, flags=0xCF
  dq 0x00CFF2000000FFFF

  ; TSS descriptor (filled in at runtime)
  dq 0x0000000000000000

gdtEnd:

SECTION .text.start
gdtDescriptor32:
  dw gdtEnd - gdt - 1         ; limit = size of GDT - 1 (i.e. 24-1 = 23)
  dd gdt                      ; base = linear address of GDT
