;------------------------------------------------------------------------------;
; Quantum Bootloader                                                           ;
;------------------------------------------------------------------------------;
; File:      Bootloader/Platform/PC/Bootstrap/HDD/Stage2.asm                ;
; Brief:     IA-32 stage 2 bootloader for FAT12 on ATA HDD.                   ;
; Author:    Brandon Belna (<bbelna@aol.com>)                                  ;
; Copyright: Copyright © 2025-2026 The Quantum Software Project.               ;
;            All rights reserved.                                              ;
;            Licensed under the GNU General Public License v2.0-only.          ;
;            See LICENSE.md for details.                                       ;
;------------------------------------------------------------------------------;

[BITS 16]
[ORG 0x0600]

jmp Start

; ------------------------------------------------------------------------------
; Constants
; ------------------------------------------------------------------------------

BS_BASE                         equ 0x7C00

BS_BytesPerSector               equ BS_BASE + 11
BS_SectorsPerCluster            equ BS_BASE + 13
BS_ReservedSectors              equ BS_BASE + 14
BS_NumFATs                      equ BS_BASE + 16
BS_RootEntries                  equ BS_BASE + 17
BS_SectorsPerFAT                equ BS_BASE + 22
BS_HiddenSectors                equ BS_BASE + 28

BootLoadAddress                 equ 0x5000

RootDirBuffer                   equ 0x2000
FATBuffer                       equ 0x3000

StackTop                        equ 0x9C00

CodeSelector                    equ 0x08
DataSelector                    equ 0x10

Stage2Sectors                   equ 2

BootInfoPhysical                equ 0x8000

;-------------------------------------------------------------------------------
; Data
;-------------------------------------------------------------------------------

BootDrive                 db 0

BytesPerSector            dw 0
SectorsPerCluster         db 0
ReservedSectors           dw 0
NumFATs                   db 0
RootDirEntries            dw 0
SectorsPerFAT             dw 0
HiddenSectors             dd 0

RootDirSectors            dw 0
FirstRootSector           dw 0
FirstDataSector           dw 0

CurrentCluster            dw 0
SavedFirstCluster         dw 0

BootSizeLow               dw 0
BootSizeHigh              dw 0
BootSectors               dw 0
BootDestLinear            dd 0

BootName                  db 'B','O','O','T',' ',' ',' ',' ','Q','B','N'

FatalErrorMsg             db 0x0A, ":( ! ", 0
DiskErrorMsg              db "Disk error", 0x0A, 0
FATErrorMsg               db "FAT error", 0x0A, 0
NoBootMsg                 db "BOOT.QBN not found", 0x0A, 0

; DAP for INT 13h extensions
DAP:
  .Size     db 16
  .Reserved db 0
  .Count    dw 0
  .Offset   dw 0
  .Segment  dw 0
  .LBA_Lo   dd 0
  .LBA_Hi   dd 0

%include "VGA.inc"

; ------------------------------------------------------------------------------
; FATAL_ERROR
; ------------------------------------------------------------------------------
%macro FATAL_ERROR 1
  PRINT FatalErrorMsg
  PRINT %1

  jmp Start.Hang
%endmacro

; ------------------------------------------------------------------------------
; Start
; ------------------------------------------------------------------------------
Start:
  cli
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, StackTop
  sti

  mov [BootDrive], dl

  .ReadDisk:
    call SetupFromBPB
    call LoadFAT

  .LoadBoot:
    call FindBoot
    call LoadBoot

  .StoreBootInfo:
    call StoreBootDrive

  .Done:
    call EnterProtectedMode

  .Hang:
    hlt
    jmp .Hang

; ------------------------------------------------------------------------------
; SetupFromBPB
; ------------------------------------------------------------------------------
SetupFromBPB:
  mov ax, [BS_BytesPerSector]
  mov [BytesPerSector], ax

  mov al, [BS_SectorsPerCluster]
  mov [SectorsPerCluster], al

  mov ax, [BS_ReservedSectors]
  mov [ReservedSectors], ax

  mov al, [BS_NumFATs]
  mov [NumFATs], al

  mov ax, [BS_RootEntries]
  mov [RootDirEntries], ax

  mov ax, [BS_SectorsPerFAT]
  mov [SectorsPerFAT], ax

  mov eax, [BS_HiddenSectors]
  mov [HiddenSectors], eax

  ; RootDirSectors = ceil(RootDirEntries * 32 / BytesPerSector)
  mov ax, [RootDirEntries]
  mov cx, 32
  mul cx
  mov bx, [BytesPerSector]
  dec bx
  add ax, bx
  adc dx, 0
  inc bx
  mov bx, [BytesPerSector]
  div bx
  mov [RootDirSectors], ax

  ; FirstRootSector = ReservedSectors + NumFATs * SectorsPerFAT
  mov dl, [NumFATs]
  xor dh, dh
  mov ax, [SectorsPerFAT]
  mul dx
  add ax, [ReservedSectors]
  mov [FirstRootSector], ax

  ; FirstDataSector = FirstRootSector + RootDirSectors
  mov ax, [RootDirSectors]
  add ax, [FirstRootSector]
  mov [FirstDataSector], ax
  ret

; ------------------------------------------------------------------------------
; LoadFAT
; ------------------------------------------------------------------------------
LoadFAT:
  mov ax, [ReservedSectors]
  mov cx, [SectorsPerFAT]
  mov si, FATBuffer

  .Loop:
    push cx
    push si

    ; read one sector at partition-relative LBA = AX
    mov bx, si
    call ReadSectorLBA

    pop si
    pop cx

    add si, 512
    inc ax
    loop .Loop
    ret

; ------------------------------------------------------------------------------
; FindBoot
; ------------------------------------------------------------------------------
FindBoot:
  mov ax, [FirstRootSector]
  mov cx, [RootDirSectors]

  .NextRootSector:
    push cx

    mov bx, RootDirBuffer
    call ReadSectorLBA

    pop cx

    mov si, RootDirBuffer
    mov bx, 16

  .ScanEntry:
    mov al, [si]
    cmp al, 0x00
    je .NoMoreEntries

    cmp al, 0xE5
    je .NextEntry

    mov al, [si + 11]
    test al, 0x08
    jnz .NextEntry
    test al, 0x10
    jnz .NextEntry

    push si
    mov di, BootName
    push cx
    mov cx, 11
    repe cmpsb
    pop cx
    pop si
    je .Found

  .NextEntry:
    add si, 32
    dec bx
    jnz .ScanEntry

    inc ax
    loop .NextRootSector
    jmp .NoMoreEntries

  .NoMoreEntries:
    FATAL_ERROR NoBootMsg

  .Found:
    mov ax, [si + 26]
    cmp ax, 2
    jb .BadCluster

    mov [CurrentCluster], ax
    mov [SavedFirstCluster], ax

    mov ax, [si + 28]
    mov [BootSizeLow], ax
    mov ax, [si + 30]
    mov [BootSizeHigh], ax

    mov ax, [BootSizeLow]
    mov dx, [BootSizeHigh]
    mov bx, [BytesPerSector]
    dec bx
    add ax, bx
    adc dx, 0
    inc bx
    div bx
    mov [BootSectors], ax

    ret

  .BadCluster:
    FATAL_ERROR FATErrorMsg

; ------------------------------------------------------------------------------
; LoadBoot
; ------------------------------------------------------------------------------
LoadBoot:
  mov dword [BootDestLinear], BootLoadAddress

  mov cx, [BootSectors]
  cmp cx, 0
  je .Done

  mov ax, [SavedFirstCluster]
  mov si, ax

  .LoadCluster:
    cmp cx, 0
    je .Done

    mov ax, si
    cmp ax, 0x0FF8
    jae .Done

    cmp ax, 2
    jb .FATError

    push cx
    call ClusterToLBA
    mov dx, ax
    pop cx

    xor ax, ax
    mov al, [SectorsPerCluster]
    mov di, ax
    cmp di, cx
    jbe .SectorsOk
    mov di, cx

  .SectorsOk:

  .ClusterSectorLoop:
    cmp di, 0
    je .NextCluster

    push dx
    push di
    push cx

    mov eax, [BootDestLinear]
    mov bx, ax
    and bx, 0x000F
    shr eax, 4
    mov es, ax

    mov ax, dx
    call ReadSectorLBA

    mov bx, [BytesPerSector]
    add word [BootDestLinear], bx
    adc word [BootDestLinear + 2], 0

    pop cx
    pop di
    pop dx

    inc dx
    dec di
    dec cx
    jmp .ClusterSectorLoop

  .NextCluster:
    mov ax, si
    call GetNextCluster
    mov si, ax
    jmp .LoadCluster

  .Done:
    ret

  .FATError:
    FATAL_ERROR FATErrorMsg

; ------------------------------------------------------------------------------
; ClusterToLBA
; ------------------------------------------------------------------------------
ClusterToLBA:
  sub ax, 2
  xor dx, dx
  xor cx, cx
  mov cl, [SectorsPerCluster]
  mul cx
  add ax, [FirstDataSector]
  ret

; ------------------------------------------------------------------------------
; GetNextCluster
; ------------------------------------------------------------------------------
GetNextCluster:
  push bx
  push cx
  push dx
  push si

  mov dx, ax
  mov cx, ax
  shr cx, 1
  add cx, ax

  mov si, FATBuffer
  add si, cx

  mov ax, [si]

  test dl, 1
  jnz .Odd

  and ax, 0x0FFF
  jmp .Done

  .Odd:
    shr ax, 4
    and ax, 0x0FFF

  .Done:
    pop si
    pop dx
    pop cx
    pop bx
    ret

; ------------------------------------------------------------------------------
; ReadSectorLBA
; ------------------------------------------------------------------------------
; Brief: Reads a sector using INT 13h extensions (LBA).
; Inputs:
;   - AX: Partition-relative LBA of the sector to read.
;   - ES:BX: Pointer to buffer to read into.
; ------------------------------------------------------------------------------
ReadSectorLBA:
  push eax
  push bx

  ; convert partition-relative LBA to absolute LBA
  movzx eax, ax
  add eax, [HiddenSectors]

  ; fill DAP
  mov byte [DAP.Size], 16
  mov byte [DAP.Reserved], 0
  mov word [DAP.Count], 1
  mov [DAP.Offset], bx
  mov [DAP.Segment], es
  mov dword [DAP.LBA_Lo], eax
  mov dword [DAP.LBA_Hi], 0

  push si
  mov ah, 0x42
  mov dl, [BootDrive]
  mov si, DAP
  int 0x13
  pop si

  jc .DiskError

  pop bx
  pop eax
  ret

  .DiskError:
    pop bx
    pop eax

    FATAL_ERROR DiskErrorMsg

; ------------------------------------------------------------------------------
; StoreBootDrive
; ------------------------------------------------------------------------------
StoreBootDrive:
  xor ax, ax
  mov ds, ax
  xor eax, eax
  mov al, [BootDrive]
  mov dword [BootInfoPhysical + 4], eax
  ret

; ------------------------------------------------------------------------------
; EnableA20
; ------------------------------------------------------------------------------
EnableA20:
  in   al, 0x92
  test al, 00000010b
  jnz  .Done
  or   al, 00000010b
  out  0x92, al

  .Done:
    ret

; ------------------------------------------------------------------------------
; EnterProtectedMode
; ------------------------------------------------------------------------------
EnterProtectedMode:
  cli
  call EnableA20

  xor ax, ax
  mov ss, ax
  mov sp, StackTop

  lgdt [GDTDescriptor]

  mov eax, cr0
  or  eax, 1
  mov cr0, eax

  push word 0x08
  push word ProtectedEntry
  retf

[BITS 32]

; ------------------------------------------------------------------------------
; ProtectedEntry
; ------------------------------------------------------------------------------
ProtectedEntry:
  mov ax, DataSelector
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov fs, ax
  mov gs, ax

  mov esp, 0x9FC00

  mov esi, BootInfoPhysical

  jmp BootLoadAddress

; ------------------------------------------------------------------------------
; GDT
; ------------------------------------------------------------------------------

[BITS 16]

GDTStart:
  dq 0x0000000000000000       ; null
  dq 0x00CF9A000000FFFF       ; code: base=0, limit=4GB, RX
  dq 0x00CF92000000FFFF       ; data: base=0, limit=4GB, RW

GDTEnd:

GDTDescriptor:
  dw GDTEnd - GDTStart - 1
  dd GDTStart

times Stage2Sectors * 512 - ($-$$) db 0
