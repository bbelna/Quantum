; ------------------------------------------------------------------------------
; Quantum
; ------------------------------------------------------------------------------
; System/Boot/Common/Print.inc
; Simple BIOS TTY print routine (AH=0x0E) for error messages.
; Brandon Belna - MIT License
; ------------------------------------------------------------------------------

Print:
  lodsb
  or al, al
  jz .done
  mov ah, 0x0E
  mov bh, 0
  int 0x10
  jmp Print
.done:
  ret

; ------------------------------------------------------------------------------
; PrintHex8
; ------------------------------------------------------------------------------
; Input: AH=0 (ignored), AL = byte (0x00-0xFF)
; Prints exactly two ASCII hex digits (no CR/LF).
; Uses BIOS TTY (INT 10h/AH=0Eh) for output.
; On exit, AX is undefined; does not change any other registers.
; ------------------------------------------------------------------------------
PrintHex8:
  push ax
  push bx

  ; isolate high nibble
  mov bl, al
  and bl, 0xF0        ; BL = high nibble << 4
  shr bl, 4           ; BL = high nibble (0..0x0F)
  call HexDigitPrint  ; prints ASCII for BL (0-15)

  ; isolate low nibble
  mov bl, al
  and bl, 0x0F
  call HexDigitPrint  ; prints ASCII for BL (0-15)

  pop bx
  pop ax
  ret

; ------------------------------------------------------------------------------
; HexDigitPrint
; ------------------------------------------------------------------------------
; Input: BL = value 0..15
; Prints a single ASCII hex digit (0-9, A-F) via BIOS TTY.
; ------------------------------------------------------------------------------
HexDigitPrint:
  cmp bl, 10
  jl  .Print0to9
  ; if BL≥10, digit = 'A'+(BL-10)
  add bl, 'A' - 10
  jmp .OutGot
.Print0to9:
  add bl, '0'
.OutGot:
  mov ah, 0x0E
  mov al, bl
  mov bh, 0x00
  mov bl, 0x07
  int 0x10
  ret
