;------------------------------------------------------------------------------;
; Quantum Bootloader                                                           ;
;------------------------------------------------------------------------------;
; File:      Bootloader/Platform/PC/Bootstrap/HDD/Stage1.asm                ;
; Brief:     IA-32 stage 1 bootloader for FAT12 on ATA HDD.                   ;
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
; FAT12 BPB (HDD partition - 32 MB FAT12)
;
; The image builder writes this BPB when formatting with mkfs.vfat -F 12.
; The values below must match the mkfs.vfat geometry exactly, or be
; overwritten by the image builder's dd of Stage1 into the partition's
; VBR (the BPB at offset 3–61 is preserved by the dd skip/count trick).
;-------------------------------------------------------------------------------

OEMLabel            db 'QUANTUM '     ; 8 bytes
BytesPerSector      dw 512
SectorsPerCluster   db 16
ReservedSectors     dw 3              ; VBR + 2 reserved (stage2)
NumFATs             db 2
RootEntries         dw 512
TotalSectors16      dw 0              ; 0 = use TotalSectors32
MediaDescriptor     db 0xF8           ; fixed disk
SectorsPerFAT       dw 12
SectorsPerTrack     dw 63
NumHeads            dw 16
HiddenSectors       dd 0             ; filled by mkfs or image builder
TotalSectors32      dd 65536          ; 32 MB at 512 B/sector

; Extended BPB (needed by some BIOSes for HDD VBRs)
DriveNumber         db 0x80
Reserved1           db 0
BootSignature       db 0x29
VolumeSerialNumber  dd 0x51544D4F    ; 'QTMO'
VolumeLabel         db 'QUANTUM    '  ; 11 bytes
FileSystemType      db 'FAT12   '     ; 8 bytes

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

  ; preserve BIOS drive number
  mov [DriveNumber], dl

  call VGAClear

  ; load Stage 2 from LBA 1-2 (relative to partition start) into 0x0600
  ; we use INT 13h extensions (AH=42h) for LBA addressing
  mov word [DAP.Count], 2
  mov word [DAP.Offset], 0x0600
  mov word [DAP.Segment], 0x0000

  ; LBA = HiddenSectors + 1 (skip VBR)
  mov eax, [HiddenSectors]
  inc eax
  mov dword [DAP.LBA_Lo], eax
  mov dword [DAP.LBA_Hi], 0

  mov ah, 0x42
  mov dl, [DriveNumber]
  mov si, DAP
  int 0x13
  jc DiskError

  ; pass boot drive in DL, jump to stage 2
  mov dl, [DriveNumber]
  jmp 0x0000:0x0600

;-------------------------------------------------------------------------------
; DAP (Disk Address Packet) for INT 13h extensions
;-------------------------------------------------------------------------------
DAP:
  .Size     db 16
  .Reserved db 0
  .Count    dw 0
  .Offset   dw 0
  .Segment  dw 0
  .LBA_Lo   dd 0
  .LBA_Hi   dd 0

;-------------------------------------------------------------------------------
; Messages
;-------------------------------------------------------------------------------

ReadErrorMsg     db 0x0A, "! :( Disk read failed", 0x0A, 0

;-------------------------------------------------------------------------------
; Includes
;-------------------------------------------------------------------------------

%include "VGA.inc"

;-------------------------------------------------------------------------------
; DiskError
;-------------------------------------------------------------------------------
DiskError:
  PRINT ReadErrorMsg
  jmp Hang

;-------------------------------------------------------------------------------
; Hang
;-------------------------------------------------------------------------------
Hang:
  hlt
  jmp Hang

times 510 - ($ - $$) db 0
dw 0xAA55
