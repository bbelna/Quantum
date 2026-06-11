[BITS 32]

global IA32GDT
global IA32GDTEnd
global IA32GDTDescriptor

SECTION .text.start
IA32GDT:
  ; null descriptor (all zeros)
  dq 0x0000000000000000

  ; code segment: base=0, limit=0xFFFFF, type=0x9A, flags=0xCF
  dq 0x00CF9A000000FFFF

  ; data segment: base=0, limit=0xFFFFF, type=0x92, flags=0xCF
  dq 0x00CF92000000FFFF

  ; user code segment: base=0, limit=0xFFFFF, type=0xFA, flags=0xCF
  dq 0x00CFFA000000FFFF

  ; user data segment: base=0, limit=0xFFFFF, type=0xF2, flags=0xCF
  dq 0x00CFF2000000FFFF

  ; TSS descriptor (filled in at runtime)
  dq 0x0000000000000000

IA32GDTEnd:

SECTION .note.GNU-stack noalloc noexec nowrite
