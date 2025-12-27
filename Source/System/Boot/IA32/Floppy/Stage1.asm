;-------------------------------------------------------------------------------
; Quantum
;-------------------------------------------------------------------------------
; File: System/Boot/IA32/Floppy/Stage1.asm
; Brief: Stage 1 bootloader for 1.44MB floppy and HDD.
; Author: Brandon Belna <bbelna@aol.com>
; Copyright: (c) 2025-2026 The Quantum OS Project
; License: MIT License
;-------------------------------------------------------------------------------

[BITS 16]
[ORG 0x7C00]

%include "Constants.inc"

jmp short BootEntry
nop

;-------------------------------------------------------------------------------
; FAT12 BPB (1.44MB floppy)
;-------------------------------------------------------------------------------

OEMLabel            db 'QUANTUM '     ; 8 bytes

BytesPerSector      dw 512
SectorsPerCluster   db 1
ReservedSectors     dw 5
NumFATs             db 2
RootEntries         dw 224
TotalSectors16      dw 2880           ; 1.44MB
MediaDescriptor     db 0xF0
SectorsPerFAT       dw 9
SectorsPerTrack     dw 18
NumHeads            dw 2
HiddenSectors       dd 0
TotalSectors32      dd 0              ; 0 for floppy

BootEntry:
Boot:
  cli
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, STACK_OFFSET
  sti

  ; Determine boot medium and jump to appropriate bootloader
  cmp dl, 0x00          ; DL==0x00 → Floppy A:
  je FloppyBoot
  cmp dl, 0x01          ; DL==0x01 → Floppy B:
  je FloppyBoot
  cmp dl, 0x80          ; DL>=0x80 → HDD
  jb Unknown

;-------------------------------------------------------------------------------
; FloppyBoot
;-------------------------------------------------------------------------------
; Reads 4 sectors starting at sector 2 into 0x0000:0x0600 and jumps there.
;-------------------------------------------------------------------------------
FloppyBoot:
  mov ah, 0x02
  mov al, 4           ; read 4 consecutive sectors (stage2 size)
  mov ch, 0           ; cylinder 0
  mov dh, 0           ; head 0
  mov cl, 2           ; starting at sector 2 (LBA 1)
  mov bx, 0x0600
  int 0x13
  jc DiskError

  jmp 0x0000:0x0600

Unknown:
  mov si, UnknownMsg
  call Print
  hlt

DiskError:
  mov si, ReadErrorMsg
  call Print
.Hang:
  hlt
  jmp .Hang

UnknownMsg    db "Unknown boot medium!", 0
ReadErrorMsg  db "Disk read failed!", 0

%include "Console.inc"

times 510-($-$$) db 0
dw 0xAA55
