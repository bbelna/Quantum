;-------------------------------------------------------------------------------
; Quantum
; System/Kernel/Arch/IA32/IDT.asm
; Brandon Belna - MIT License
;-------------------------------------------------------------------------------
; Loads the IDT and provides ISR stubs.
;-------------------------------------------------------------------------------

[BITS 32]

global LoadIDT
global ISR0
extern IDTExceptionHandler

; void LoadIDT(IDTDescriptor* desc);
LoadIDT:
  mov eax, [esp + 4]  ; arg0: pointer to descriptor
  lidt [eax]
  ret

; ISR0: divide-by-zero
ISR0:
  pusha                 ; save all general-purpose regs
  push dword 0          ; error code (none for #DE, so 0)
  push dword 0          ; vector number (0)
  call IDTExceptionHandler
  add esp, 8            ; pop vector + error code
  popa                  ; restore regs
  add esp, 4            ; pop return address (we won't return)

.hang:
  hlt
  jmp .hang
