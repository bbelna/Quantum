;------------------------------------------------------------------------------
; Quantum
;------------------------------------------------------------------------------
; File: Boot/Common/Print.inc
; Description: Simple BIOS TTY print routine (AH=0x0E) for error messages.
; Author: Brandon Belna
; License: MIT
;------------------------------------------------------------------------------

Print:
  mov ah, 0x0E
.NextChar:
  lodsb
  cmp al, 0
  je .Done
  int 0x10
  jmp .NextChar
.Done:
  ret
