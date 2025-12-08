;-------------------------------------------------------------------------------
; Quantum
; System/Kernel/Arch/IA32/GDT.asm
; Brandon Belna - MIT License
;-------------------------------------------------------------------------------
; Defines a minimal GDT (null, code, data) and its descriptor for 32-bit mode.
; GDTDescriptor32 is the 6-byte [limit(2), base(4)] that `lgdt` expects.
;-------------------------------------------------------------------------------

BITS 32

global GDT
global GDTDescriptor32

SECTION .data
GDT:
  ; 1) Null descriptor (all zeros)
  dq 0x0000000000000000

  ; 2) Code segment: base=0, limit=0xFFFFF, type=0x9A, flags=0xCF
  ;    0x00CF9A000000FFFF means:
  ;      • limit_low = 0xFFFF
  ;      • base_low  = 0x0000
  ;      • base_mid  = 0x00
  ;      • access    = 0x9A   (present, ring0, code segment, executable, readable)
  ;      • flags     = 0xC    (4K granularity, 32-bit, long=0)
  ;      • limit_high=0xF
  ;      • base_high = 0x00
  dq 0x00CF9A000000FFFF

  ; 3) Data segment: base=0, limit=0xFFFFF, type=0x92, flags=0xCF
  ;    0x00CF92000000FFFF means:
  ;      • limit_low = 0xFFFF
  ;      • base_low  = 0x0000
  ;      • base_mid  = 0x00
  ;      • access    = 0x92   (present, ring0, data segment, writable)
  ;      • flags     = 0xC    (4K granularity, 32-bit)
  ;      • limit_high=0xF
  ;      • base_high = 0x00
  dq 0x00CF92000000FFFF

GDTEnd:

SECTION .data
GDTDescriptor32:
  dw GDTEnd - GDT - 1         ; limit = size of GDT - 1 (i.e. 24-1 = 23)
  dd GDT                       ; base = linear address of GDT

