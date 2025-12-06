;-------------------------------------------------------------------------------
; Quantum
;-------------------------------------------------------------------------------
; System/Boot/Floppy/Floppy.asm
; Stage 2 bootloader for 1.44MB floppy.
; Brandon Belna - MIT License
;-------------------------------------------------------------------------------

[BITS 16]
[ORG 0x0600]

jmp Start

; --------------------------------------------------------------------------
; Constants / Addresses
; --------------------------------------------------------------------------

BS_BASE             equ 0x7C00        ; boot sector still in memory

; BPB offsets
BS_BytesPerSector   equ BS_BASE + 11
BS_SectorsPerCluster equ BS_BASE + 13
BS_ReservedSectors  equ BS_BASE + 14
BS_NumFATs          equ BS_BASE + 16
BS_RootEntries      equ BS_BASE + 17
BS_SectorsPerFAT    equ BS_BASE + 22
BS_SectorsPerTrack  equ BS_BASE + 24
BS_NumHeads         equ BS_BASE + 26

; Memory layout (segment:offset with DS = ES = 0 initially)
KERNEL_LOAD_SEG     equ 0x1000        ; kernel at 0x1000:0000 = 0x00010000
KERNEL_PM_ENTRY     equ 0x00010000    ; 32-bit kernel entry linear addr

ROOTDIR_BUF         equ 0x2000        ; buffer for root dir
FAT_BUF             equ 0x3000        ; buffer for FAT

STACK_TOP           equ 0x9C00        ; simple real-mode stack

; GDT selectors
CODE_SEL            equ 0x08
DATA_SEL            equ 0x10

STAGE2_SECTORS      equ 4             ; must match stage1 AL

; --------------------------------------------------------------------------
; Data / Variables
; --------------------------------------------------------------------------

BootDrive           db 0

BytesPerSector      dw 0
SectorsPerCluster   db 0
ReservedSectors     dw 0
NumFATs             db 0
RootDirEntries      dw 0
SectorsPerFAT       dw 0
SectorsPerTrack     dw 0
NumHeads            dw 0
ClusterGuard        dw 0

RootDirSectors      dw 0
FirstRootSector     dw 0
FirstDataSector     dw 0

CurrentCluster      dw 0
TempLBA             dw 0

KernelSizeLow       dw 0
KernelSizeHigh      dw 0

KERNEL_NAME         db 'Q','K','R','N','L',' ',' ',' ','Q','X',' '

Msg_NoKernel        db "Kernel QKRNL.QX not found!", 0
Msg_DiskError       db "Disk read error!", 0
Msg_FATError        db "FAT error!", 0

Start:
  cli
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, STACK_TOP
  sti

  mov si, Stage2Msg
  call Print

  mov [BootDrive], dl          ; preserve BIOS drive

  call SetupFromBPB
  mov si, MsgAfterSetup
  call Print

  call LoadFAT
  mov si, MsgAfterFAT
  call Print

  call FindKernel
  mov si, MsgAfterFind
  call Print

  call LoadKernel              ; loads to 0x00010000
  mov si, MsgAfterLoad
  call Print

  ; Everything loaded, now go to protected mode
  call EnterProtectedMode

.Hang:
  hlt
  jmp .Hang

Stage2Msg      db "Stage 2...", 0
MsgAfterSetup  db "Setup OK", 0
MsgAfterFAT    db "FAT OK", 0
MsgAfterFind   db "Kernel found", 0
MsgAfterLoad   db "Kernel loaded", 0

SetupFromBPB:
  ; mov ax, [BS_BytesPerSector]
  ; mov [BytesPerSector], ax
  ; mov al, [BS_SectorsPerCluster]
  ; mov [SectorsPerCluster], al
  ; mov ax, [BS_ReservedSectors]
  ; mov [ReservedSectors], ax
  ; mov al, [BS_NumFATs]
  ; mov [NumFATs], al
  ; mov ax, [BS_RootEntries]
  ; mov [RootDirEntries], ax
  ; mov ax, [BS_SectorsPerFAT]
  ; mov [SectorsPerFAT], ax

  ; --- Hard-coded floppy geometry and FAT12 layout ---
  mov word [BytesPerSector],    512
  mov byte [SectorsPerCluster], 1
  mov word [ReservedSectors],   5
  mov byte [NumFATs],           2
  mov word [RootDirEntries],    224
  mov word [SectorsPerFAT],     9

  ; Geometry for CHS↔LBA:
  mov word [SectorsPerTrack],   18
  mov word [NumHeads],          2

  ; root_dir_sectors = ceil(RootDirEntries * 32 / BytesPerSector)
  mov ax, [RootDirEntries]
  mov cx, 32
  mul cx                     ; DX:AX = entries * 32

  mov bx, [BytesPerSector]
  dec bx
  add ax, bx
  adc dx, 0

  inc bx
  mov bx, [BytesPerSector]
  div bx                     ; AX = RootDirSectors
  mov [RootDirSectors], ax

  ; FirstRootSector = ReservedSectors + NumFATs * SectorsPerFAT
  mov dl, [NumFATs]
  xor dh, dh                 ; DX = NumFATs
  mov ax, [SectorsPerFAT]
  mul dx                     ; AX = SectorsPerFAT * NumFATs
  add ax, [ReservedSectors]
  mov [FirstRootSector], ax

  ; FirstDataSector = FirstRootSector + RootDirSectors
  mov ax, [RootDirSectors]
  add ax, [FirstRootSector]
  mov [FirstDataSector], ax
  ret

LoadFAT:
  mov ax, [ReservedSectors]   ; first FAT sector
  mov [TempLBA], ax

  mov cx, [SectorsPerFAT]
  mov si, FAT_BUF

.LoadFAT_Loop:
  mov bx, si
  mov ax, [TempLBA]
  call ReadSectorLBA

  add si, 512
  inc word [TempLBA]
  loop .LoadFAT_Loop
  ret

FindKernel:
  mov ax, [FirstRootSector]
  mov [TempLBA], ax

  mov cx, [RootDirSectors]      ; number of root dir sectors

.NextRootSector:
  mov bx, ROOTDIR_BUF
  mov ax, [TempLBA]
  call ReadSectorLBA

  mov si, ROOTDIR_BUF
  mov bx, 16                    ; 512 / 32 = 16 entries per sector

.ScanEntry:
  mov al, [si]                  ; first byte
  cmp al, 0x00
  je .NoMoreEntries             ; end of directory

  cmp al, 0xE5
  je .NextEntry                 ; deleted

  mov al, [si + 11]             ; attribute
  test al, 0x08
  jnz .NextEntry                ; volume label
  test al, 0x10
  jnz .NextEntry                ; directory

  ; compare 11-byte name
  push si
  mov di, KERNEL_NAME
  mov cx, 11
  repe cmpsb                    ; compare DS:SI with ES:DI (DS=ES=0)
  pop si
  je .FoundKernel

.NextEntry:
  add si, 32                    ; next directory entry
  dec bx                        ; decrement entry counter
  jnz .ScanEntry

  inc word [TempLBA]            ; next root dir sector
  loop .NextRootSector
  jmp .NoMoreEntries

.NoMoreEntries:
  mov si, Msg_NoKernel
  call Print
  jmp Start.Hang

.FoundKernel:
  mov ax, [si + 26]           ; first cluster (WORD)

  cmp ax, 2                   ; 0,1 are reserved
  jb .BadCluster

  cmp ax, 0x0FF0              ; absurd for a small floppy
  ja .BadCluster

  mov [CurrentCluster], ax

  mov ax, [si + 28]           ; file size low word
  mov [KernelSizeLow], ax
  mov ax, [si + 30]           ; file size high word
  mov [KernelSizeHigh], ax
  ret

.BadCluster:
  mov si, Msg_NoKernel
  call Print
  jmp Start.Hang

LoadKernel:
  mov ax, KERNEL_LOAD_SEG
  mov es, ax
  xor bx, bx

  ; Assume kernel starts at cluster 2, contiguous
  mov ax, [FirstDataSector]   ; LBA of cluster 2
  mov [TempLBA], ax

  ; We know the file is 790 bytes → 2 sectors
  mov cx, 2

.ClusterSectorLoop:
  mov ax, [TempLBA]
  call ReadSectorLBA          ; read into ES:BX

  mov ax, [BytesPerSector]
  add bx, ax
  inc word [TempLBA]
  loop .ClusterSectorLoop

  ret

GetNextCluster:
  push bx
  push dx
  push si

  mov dx, ax                 ; DX = N
  mov bx, ax
  shr bx, 1
  add bx, dx                 ; BX = N + N/2 = 3N/2

  mov si, FAT_BUF
  add si, bx                 ; SI = &FAT[N*1.5]

  mov ax, [si]

  test dl, 1
  jz .even

  ; odd cluster
  shr ax, 4
  jmp .done

.even:
  and ax, 0x0FFF

.done:
  pop si
  pop dx
  pop bx
  ret

ReadSectorLBA:
  ; IN:  AX = LBA, ES:BX = dest
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

  ; reset disk
  mov dl, [BootDrive]
  mov ah, 0x00
  int 0x13

  ; read
  mov dl, [BootDrive]
  mov ah, 0x02
  mov al, 1
  ; ES:BX from caller
  int 0x13
  jc .disk_error

  pop dx
  pop cx
  pop bx
  pop ax
  ret

.disk_error:
  pop dx
  pop cx
  pop bx
  pop ax

  mov si, Msg_DiskError
  call Print
  jmp Start.Hang

EnableA20:
  ; Fast A20 (port 0x92)
  in   al, 0x92
  test al, 00000010b
  jnz  .done
  or   al, 00000010b
  out  0x92, al

.done:
  ret

EnterProtectedMode:
  cli
  call EnableA20

  lgdt [gdt_descriptor]

  mov eax, cr0
  or  eax, 1
  mov cr0, eax

  ; far jump to flush prefetch, into 32-bit code
  jmp CODE_SEL:ProtectedEntry

[BITS 32]

ProtectedEntry:
  mov ax, DATA_SEL
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov fs, ax
  mov gs, ax

  mov esp, 0x90000        ; 32-bit stack (under 1MB for now)

  ; Optionally pass boot drive in a register (e.g., BL)
  movzx ebx, byte [BootDrive]

  ; Jump to 32-bit kernel entry
  jmp KERNEL_PM_ENTRY

; --------------------------------------------------------------------------
; GDT
; --------------------------------------------------------------------------

[BITS 16]

gdt_start:
  dq 0x0000000000000000       ; null
  dq 0x00CF9A000000FFFF       ; code: base=0, limit=4GB, RX
  dq 0x00CF92000000FFFF       ; data: base=0, limit=4GB, RW
gdt_end:

gdt_descriptor:
  dw gdt_end - gdt_start - 1
  dd gdt_start

%include "Print.inc"

times STAGE2_SECTORS*512 - ($-$$) db 0
