;------------------------------------------------------------------------------;
; Quantum Bootloader                                                           ;
;------------------------------------------------------------------------------;
; File:      Bootloader/Platform/PC/Bootstrap/Floppy/VGA.inc                ;
; Brief:     IA-32 BIOS VGA text-mode routines and definitions.                ;
; Author:    Brandon Belna (<bbelna@aol.com>)                                  ;
; Copyright: Copyright © 2025-2026 The Quantum Software Project.               ;
;            All rights reserved.                                              ;
;            Licensed under the GNU General Public License v2.0-only.          ;
;            See LICENSE.md for details.                                       ;
;------------------------------------------------------------------------------;

; ------------------------------------------------------------------------------
; Print
; ------------------------------------------------------------------------------
; Brief:  Prints a zero-terminated string via BIOS TTY (INT 10h/AH=0Eh).
; Input:  DS:SI zero-terminated string.
; Output: All registers preserved (AX, BX, CX, DX, SI, DI).
; ------------------------------------------------------------------------------
Print:
  push ax
  push bx
  push cx
  push dx
  push si
  push di

  xor bh, bh          ; page 0
  mov bl, 0x07

  .PrintLoop:
    lodsb               ; AL = [DS:SI], SI++
    test al, al
    jz .Done

    ; handle '\n' as CRLF
    cmp al, 0x0A
    jne .Put

    mov ah, 0x0E
    mov al, 0x0D
    int 0x10

    mov al, 0x0A
    int 0x10

    jmp .PrintLoop

  .Put:
    ; get current cursor position -> DH=row, DL=col
    mov ah, 0x03
    int 0x10

    ; write character AL with attribute BL at current cursor
    mov ah, 0x09
    mov cx, 1
    int 0x10

    ; advance cursor by 1 column
    inc dl
    mov ah, 0x02
    int 0x10

    jmp .PrintLoop

  .Done:
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; ------------------------------------------------------------------------------
; PRINT
; ------------------------------------------------------------------------------
; Brief:  Safely prints a zero-terminated string via BIOS TTY (INT 10h/AH=0Eh)
; Input:  DS:SI zero-terminated string.
; Output: All registers preserved (AX, BX, CX, DX, SI, DI).
; ------------------------------------------------------------------------------
%macro PRINT 1
  push si
  mov si, %1
  call Print
  pop si
%endmacro

; ------------------------------------------------------------------------------
; VGAClear
; ------------------------------------------------------------------------------
; Brief: Clears the text-mode console via BIOS INT 10h/AH=00h.
; ------------------------------------------------------------------------------
VGAClear:
  mov ax, 0x0003
  int 0x10
  ret
