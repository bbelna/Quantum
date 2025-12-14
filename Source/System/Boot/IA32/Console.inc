; ------------------------------------------------------------------------------
; Quantum
; System/Boot/IA32/Console.inc
; (c) 2025 Brandon Belna - MIT LIcense
; ------------------------------------------------------------------------------
; IA32 BIOS console routines.
; ------------------------------------------------------------------------------

; ------------------------------------------------------------------------------
; Print
; ------------------------------------------------------------------------------
; Prints a zero-terminated string via BIOS TTY (INT 10h/AH=0Eh).
; In: DS:SI -> zero-terminated string
; Out: all registers preserved (AX, BX, CX, DX, SI, DI)
; ------------------------------------------------------------------------------
Print:
  push ax
  push bx
  push cx
  push dx
  push si
  push di

.print_loop:
  lodsb                  ; AL = [DS:SI], SI++
  test al, al
  jz .done               ; if AL == 0 → end of string

  mov ah, 0x0E           ; teletype output
  mov bh, 0x00           ; page 0
  mov bl, 0x07           ; attribute (ignored in text TTY, but fine)
  int 0x10

  jmp .print_loop

.done:
  pop di
  pop si
  pop dx
  pop cx
  pop bx
  pop ax
  ret

; ------------------------------------------------------------------------------
; PrintHex8
; ------------------------------------------------------------------------------
; Prints exactly two ASCII hex digits (no CR/LF).
; Uses BIOS TTY (INT 10h/AH=0Eh) for output.
; On exit, AX is undefined; does not change any other registers.
; In: AH=0 (ignored), AL = byte (0x00-0xFF)
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
; Prints a single ASCII hex digit (0-9, A-F) via BIOS TTY.
; In: BL = value 0..15
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

;------------------------------------------------------------------------------
; DebugPrint
;------------------------------------------------------------------------------
; Prints a zero-terminated string at DS:SI to QEMU debug console (port 0xE9)
; In: DS:SI -> null-terminated string
; Out: all general registers preserved
;------------------------------------------------------------------------------
DebugPrint:
  push ax
  push bx
  push cx
  push dx
  push si
  push di

.dbg_loop:
  lodsb                  ; AL = [DS:SI], SI++
  test al, al
  jz .done               ; if 0, end

  push dx
  mov dx, 0x00E9
  out dx, al
  pop dx

  jmp .dbg_loop

.done:
  pop di
  pop si
  pop dx
  pop cx
  pop bx
  pop ax
  ret

; ------------------------------------------------------------------------------
; DBG macro
; ------------------------------------------------------------------------------
; Safely prints a zero-terminated string at DS:SI to QEMU debug console
; (port 0xE9). i.e., preserves SI.
; In: %1 = pointer to null-terminated string
; Out: all general registers preserved
; ------------------------------------------------------------------------------
%macro DBG 1
  push si
  mov si, %1
  call DebugPrint
  pop si
%endmacro

; ------------------------------------------------------------------------------
; ClearConsole
; ------------------------------------------------------------------------------
; Clears the text-mode console via BIOS INT 10h/AH=00h.
; ------------------------------------------------------------------------------
ClearConsole:
  mov ax, 0x0003
  int 0x10
  ret
