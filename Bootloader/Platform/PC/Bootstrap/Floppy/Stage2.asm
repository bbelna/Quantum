;------------------------------------------------------------------------------;
; Quantum Bootloader                                                           ;
;------------------------------------------------------------------------------;
; File:      Bootloader/Platform/PC/Bootstrap/Floppy/Stage2.asm             ;
; Brief:     IA-32 stage 2 bootloader for 1.44MB floppy.                       ;
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
BS_SectorsPerTrack              equ BS_BASE + 24
BS_NumHeads                     equ BS_BASE + 26

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
SectorsPerTrack           dw 0
NumHeads                  dw 0
ClusterGuard              dw 0

RootDirSectors            dw 0
FirstRootSector           dw 0
FirstDataSector           dw 0

CurrentCluster            dw 0
SavedFirstCluster         dw 0
TempLBA                   dw 0

BootSizeLow               dw 0
BootSizeHigh              dw 0
BootSectors               dw 0
BootDestLinear            dd 0

BootName                  db 'B','O','O','T',' ',' ',' ',' ','Q','B','N'

FatalErrorMsg             db 0x0A, ":( ! ", 0
DiskErrorMsg              db "Disk error", 0x0A, 0
FATErrorMsg               db "FAT error", 0x0A, 0
NoBootMsg                 db "BOOT.QBN not found", 0x0A, 0

%include "VGA.inc"

; ------------------------------------------------------------------------------
; FATAL_ERROR
; ------------------------------------------------------------------------------
; Brief: Prints an error message and halts.
; Input: Zero-terminated string.
; ------------------------------------------------------------------------------
%macro FATAL_ERROR 1
  PRINT FatalErrorMsg
  PRINT %1

  jmp Start.Hang
%endmacro

; ------------------------------------------------------------------------------
; Start
; ------------------------------------------------------------------------------
; Brief: Entry point for stage 2 bootloader.
; ------------------------------------------------------------------------------
Start:
  ; initialize segments and stack
  cli
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, StackTop
  sti

  ; preserve BIOS drive
  mov [BootDrive], dl

  .ReadDisk:
    call ResetDisk
    call SetupFromBPB
    call LoadFAT

  .LoadBoot:
    call FindBoot
    call LoadBoot              ; loads to 0x5000

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
; Brief: Sets up variables from the BIOS Parameter Block (BPB).
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

  mov ax, [BS_SectorsPerTrack]
  mov [SectorsPerTrack], ax

  mov ax, [BS_NumHeads]
  mov [NumHeads], ax

  ; RootDirSectors = ceil(RootDirEntries * 32 / BytesPerSector)
  mov ax, [RootDirEntries]
  mov cx, 32
  mul cx                       ; DX:AX = entries * 32

  mov bx, [BytesPerSector]
  dec bx
  add ax, bx
  adc dx, 0
  inc bx
  mov bx, [BytesPerSector]
  div bx                       ; AX = RootDirSectors
  mov [RootDirSectors], ax

  ; FirstRootSector = ReservedSectors + NumFATs * SectorsPerFAT
  mov dl, [NumFATs]
  xor dh, dh                   ; DX = NumFATs
  mov ax, [SectorsPerFAT]
  mul dx                       ; AX = NumFATs * SectorsPerFAT
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
; Brief: Loads the File Allocation Table (FAT) into memory.
; ------------------------------------------------------------------------------
LoadFAT:
  mov ax, [ReservedSectors]    ; first FAT sector
  mov [TempLBA], ax

  mov cx, [SectorsPerFAT]      ; number of sectors in one FAT
  mov si, FATBuffer

  .LoadFAT_Loop:
    mov bx, si                 ; ES is 0, so ES:BX = 0:si
    mov ax, [TempLBA]
    call ReadSectorLBA

    add si, 512
    inc word [TempLBA]
    loop .LoadFAT_Loop
    ret

; ------------------------------------------------------------------------------
; FindBoot
; ------------------------------------------------------------------------------
; Brief: Finds the Boot file in the root directory and sets up
;        parameters for loading it.
; ------------------------------------------------------------------------------
FindBoot:
  mov ax, [FirstRootSector]
  mov [TempLBA], ax

  mov cx, [RootDirSectors]

  .NextRootSector:
    mov bx, RootDirBuffer
    mov ax, [TempLBA]
    call ReadSectorLBA

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
    mov cx, 11
    repe cmpsb
    pop si
    je .Found

  .NextEntry:
    add si, 32
    dec bx
    jnz .ScanEntry

    inc word [TempLBA]
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
; Brief: Loads the Boot image from the disk into memory at BootLoadAddress.
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
; Brief: Converts a cluster number to a logical block address (LBA).
; Inputs:
;   - AX: Cluster number.
; Outputs:
;   - AX: LBA corresponding to the start of the cluster.
; ------------------------------------------------------------------------------
ClusterToLBA:
  sub ax, 2                      ; (cluster - 2)
  xor dx, dx
  xor cx, cx
  mov cl, [SectorsPerCluster]
  mul cx                         ; AX = (cluster-2) * SPC
  add ax, [FirstDataSector]      ; + first data sector
  ret

; ------------------------------------------------------------------------------
; GetNextCluster
; ------------------------------------------------------------------------------
; Brief: Retrieves the next cluster number from the FAT.
; Inputs:
;   - AX: Current cluster number.
; Outputs:
;   - AX: Next cluster number.
; ------------------------------------------------------------------------------
GetNextCluster:
  push bx
  push cx
  push dx
  push si

  mov dx, ax                    ; DX = N (we'll use DL to test odd/even)

  ; offset = floor(3 * N / 2)
  mov cx, ax
  shr cx, 1                     ; CX = N/2 (floor)
  add cx, ax                    ; CX = N + N/2 = 3N/2

  mov si, FATBuffer
  add si, cx                    ; SI = FAT + offset

  ; Read 16 bits from FAT[offset]
  mov ax, [si]

  ; Even or odd cluster?
  test dl, 1
  jnz .Odd                      ; odd cluster

  ; even cluster N: low 12 bits
  and ax, 0x0FFF
  jmp .Done

  .Odd:
    ; odd cluster N: high 12 bits
    shr ax, 4
    and ax, 0x0FFF

  .Done:
    pop si
    pop dx
    pop cx
    pop bx
    ret

; ------------------------------------------------------------------------------
; ResetDisk
; ------------------------------------------------------------------------------
; Brief: Resets the disk system.
; ------------------------------------------------------------------------------
ResetDisk:
  mov dl, [BootDrive]
  mov ah, 0x00
  int 0x13
  ret

; ------------------------------------------------------------------------------
; ReadSectorLBA
; ------------------------------------------------------------------------------
; Brief: Reads a sector from the disk using LBA addressing.
; Inputs:
;   - AX: LBA of the sector to read.
;   - ES:BX: Pointer to buffer to read into.
; Outputs:
;   - CF set on error.
; ------------------------------------------------------------------------------
ReadSectorLBA:
  push ax
  push bx
  push cx
  push dx

  ; Convert LBA to CHS

  ; track = LBA / SectorsPerTrack
  mov dx, 0
  div word [SectorsPerTrack]   ; AX = track, DX = sector-1
  inc dx                       ; sector (1-based)
  mov cx, dx                   ; CL = sector

  ; cylinder/head
  mov dx, 0
  div word [NumHeads]          ; AX = cylinder, DX = head

  mov dh, dl                   ; DH = head
  mov ch, al                   ; CH = cylinder low
  shl ah, 6
  or  cl, ah                   ; sector | (cyl_high << 6)

  ; read
  mov dl, [BootDrive]
  mov ah, 0x02
  mov al, 1

  ; ES:BX from caller
  int 0x13
  jc .DiskError

  pop dx
  pop cx
  pop bx
  pop ax
  ret

  .DiskError:
    pop dx
    pop cx
    pop bx
    pop ax

    FATAL_ERROR DiskErrorMsg

; ------------------------------------------------------------------------------
; StoreBootDrive
; ------------------------------------------------------------------------------
; Brief: Stores the BIOS boot drive number in BootInfo for Boot to read.
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
; Brief: Enables the A20 line using the fast method (port 0x92).
; ------------------------------------------------------------------------------
EnableA20:
  ; Fast A20 (port 0x92)
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
; Brief: Switches the CPU to protected mode and jumps to the 32-bit kernel.
; ------------------------------------------------------------------------------
EnterProtectedMode:
  cli
  call EnableA20

  ; reset real-mode stack to a known location before switching
  xor ax, ax
  mov ss, ax
  mov sp, StackTop

  lgdt [GDTDescriptor]

  mov eax, cr0
  or  eax, 1
  mov cr0, eax

  ; far jump to flush prefetch, into 32-bit code using stack + retf
  push word 0x08
  push word ProtectedEntry
  retf

[BITS 32]

; ------------------------------------------------------------------------------
; ProtectedEntry
; ------------------------------------------------------------------------------
; Brief: Jumps to the 32-bit Boot entry point.
; Inputs:
;   - ESI: Physical pointer to BootInfo structure.
; ------------------------------------------------------------------------------
ProtectedEntry:
  mov ax, DataSelector
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov fs, ax
  mov gs, ax

  mov esp, 0x9FC00        ; 32-bit stack (top of conventional memory)

  ; Pass BootInfo physical pointer in ESI
  mov esi, BootInfoPhysical

  ; Jump to 32-bit Boot entry point
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
