[BITS 32]

global IA32GDTDescriptor
extern IA32GDT

SECTION .text.start
IA32GDTDescriptor:
  dw 47                               ; limit = 6 entries * 8 bytes - 1
  dd IA32GDT                          ; base = linear address of GDT

SECTION .note.GNU-stack noalloc noexec nowrite
