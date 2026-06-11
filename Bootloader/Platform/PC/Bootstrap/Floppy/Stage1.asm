;------------------------------------------------------------------------------;
; Quantum Bootloader                                                           ;
;------------------------------------------------------------------------------;
; File:      Bootloader/Platform/PC/Bootstrap/Floppy/Stage1.asm             ;
; Brief:     IA-32 stage 1 bootloader for 1.44MB floppy.                       ;
; Author:    Brandon Belna (<bbelna@aol.com>)                                  ;
; Copyright: Copyright © 2025-2026 The Quantum Software Project.               ;
;            All rights reserved.                                              ;
;            Licensed under the GNU General Public License v2.0-only.          ;
;            See LICENSE.md for details.                                       ;
;------------------------------------------------------------------------------;

[BITS 16]
[ORG 0x7C00]

%define STACK_OFFSET 0x7C00

jmp short Boot
nop

;-------------------------------------------------------------------------------
; FAT12 BPB (1.44MB floppy)
;-------------------------------------------------------------------------------

OEMLabel            db 'QUANTUM '     ; 8 bytes
BytesPerSector      dw 512
SectorsPerCluster   db 1
ReservedSectors     dw 3
NumFATs             db 2
RootEntries         dw 224
TotalSectors16      dw 2880           ; 1.44MB
MediaDescriptor     db 0xF0
SectorsPerFAT       dw 9
SectorsPerTrack     dw 18
NumHeads            dw 2
HiddenSectors       dd 0
TotalSectors32      dd 0              ; 0 for floppy

; ------------------------------------------------------------------------------
; Boot
; ------------------------------------------------------------------------------
; Brief: Initial entry point for stage 1 bootloader.
; ------------------------------------------------------------------------------
Boot:
  cli
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, STACK_OFFSET
  sti

  call VGAClear

  ; determine boot medium and jump to appropriate bootloader

  cmp dl, 0x00          ; floppy A
  je Floppy

  cmp dl, 0x01          ; floppy B
  je Floppy

  cmp dl, 0x80          ; HDD
  jb Unknown

;-------------------------------------------------------------------------------
; Messages
;-------------------------------------------------------------------------------

UnknownMsg       db 0x0A, "! :( Unknown boot medium", 0x0A, 0
ReadErrorMsg     db 0x0A, "! :( Disk read failed", 0x0A, 0

;-------------------------------------------------------------------------------
; Includes
;-------------------------------------------------------------------------------

%include "VGA.inc"

;-------------------------------------------------------------------------------
; Floppy
;-------------------------------------------------------------------------------
; Brief: Reads 4 sectors starting at sector 2 into 0x0000:0x0600 and jumps
;        there.
;-------------------------------------------------------------------------------
Floppy:
  mov ah, 0x02
  mov al, 2           ; read 2 consecutive sectors (stage2 size)
  mov ch, 0           ; cylinder 0
  mov dh, 0           ; head 0
  mov cl, 2           ; starting at sector 2 (LBA 1)
  mov bx, 0x0600
  int 0x13
  jc DiskError

  jmp 0x0000:0x0600

;-------------------------------------------------------------------------------
; Unknown
;-------------------------------------------------------------------------------
; Brief: Handles unknown boot medium error.
;-------------------------------------------------------------------------------
Unknown:
  PRINT UnknownMsg
  jmp Hang

;-------------------------------------------------------------------------------
; DiskError
;-------------------------------------------------------------------------------
; Brief: Handles disk read error.
;-------------------------------------------------------------------------------
DiskError:
  PRINT ReadErrorMsg
  jmp Hang

;-------------------------------------------------------------------------------
; Hang
;-------------------------------------------------------------------------------
; Brief: Halts the CPU indefinitely.
;-------------------------------------------------------------------------------
Hang:
  hlt
  jmp Hang

times 510 - ($ - $$) db 0
dw 0xAA55
