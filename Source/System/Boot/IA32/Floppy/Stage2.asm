;-------------------------------------------------------------------------------
; Quantum
; System/Boot/IA32/Floppy/Stage2.asm
; (c) 2025 Brandon Belna - MIT LIcense
;-------------------------------------------------------------------------------
; Stage 2 bootloader for 1.44MB floppy.
;-------------------------------------------------------------------------------

[BITS 16]
[ORG 0x0600]

jmp Start

; --------------------------------------------------------------------------
; Constants / Addresses
; --------------------------------------------------------------------------

BS_BASE                 equ 0x7C00        ; boot sector still in memory

; BPB offsets
BS_BytesPerSector       equ BS_BASE + 11
BS_SectorsPerCluster    equ BS_BASE + 13
BS_ReservedSectors      equ BS_BASE + 14
BS_NumFATs              equ BS_BASE + 16
BS_RootEntries          equ BS_BASE + 17
BS_SectorsPerFAT        equ BS_BASE + 22
BS_SectorsPerTrack      equ BS_BASE + 24
BS_NumHeads             equ BS_BASE + 26

; Memory layout (segment:offset with DS = ES = 0 initially)
KernelLoadSeg           equ 0x1000        ; kernel at 0x1000:0000 = 0x00010000
KernelPMEntry           equ 0x00010000    ; 32-bit kernel entry linear addr

RootDirBuffer           equ 0x2000        ; buffer for root dir
FATBuffer               equ 0x3000        ; buffer for FAT

StackTop                equ 0x9C00        ; simple real-mode stack

; GDT selectors
CodeSelector            equ 0x08
DataSelector            equ 0x10

Stage2Sectors           equ 4             ; must match stage1 AL

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
SavedFirstCluster   dw 0
TempLBA             dw 0

KernelSizeLow             dw 0
KernelSizeHigh            dw 0
KernelBytesRemainingLow   dw 0
KernelBytesRemainingHigh  dw 0
KernelSectors             dw 0
KernelSectorsRemaining    dw 0
KernelDestLinear          dd 0

KernelName        db 'K','E','R','N','E','L',' ',' ','Q','X',' '

; E820 memory map buffer (BootInfo layout)
BootInfoPhysical        equ 0x8000
BootInfoTotalBytes      equ 8 + (32 * 20)           ; entryCount/res + 32 entries
MemMapEntriesOffset     equ BootInfoPhysical + 8    ; after entryCount/reserved
MemMapEntrySize         equ 20
MemMapMaxEntries        equ 32
MemMapEntryCount        equ BootInfoPhysical        ; dword

NoKernelMsg        db "KERNEL.QX not found!", 0
DiskErrorMsg       db "Disk read error!", 0
FATErrorMsg        db "FAT error!", 0

Start:
  ; start by clearing the console to indicate stage2 has started
  call ClearConsole

  cli
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, StackTop
  sti

  mov [BootDrive], dl          ; preserve BIOS drive

  call SetupFromBPB
  call LoadFAT
  call FindKernel
  call LoadKernel              ; loads to 0x00010000
  call CollectE820
  call EnterProtectedMode

.Hang:
  hlt
  jmp .Hang

SetupFromBPB:
  ; Bytes per sector (should be 512)
  mov ax, [BS_BytesPerSector]
  mov [BytesPerSector], ax

  ; Sectors per cluster (usually 1 on 1.44MB FAT12)
  mov al, [BS_SectorsPerCluster]
  mov [SectorsPerCluster], al

  ; Reserved sectors (you have -R 5, so this should be 5)
  mov ax, [BS_ReservedSectors]
  mov [ReservedSectors], ax

  ; Number of FATs (2)
  mov al, [BS_NumFATs]
  mov [NumFATs], al

  ; Max root dir entries (224)
  mov ax, [BS_RootEntries]
  mov [RootDirEntries], ax

  ; Sectors per FAT (9)
  mov ax, [BS_SectorsPerFAT]
  mov [SectorsPerFAT], ax

  ; Geometry for CHS conversion
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

LoadFAT:
  mov ax, [ReservedSectors]    ; first FAT sector
  mov [TempLBA], ax

  mov cx, [SectorsPerFAT]      ; number of sectors in one FAT
  mov si, FATBuffer

.LoadFAT_Loop:
  mov bx, si                   ; ES is 0, so ES:BX = 0:si
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
  mov bx, RootDirBuffer
  mov ax, [TempLBA]
  call ReadSectorLBA

  mov si, RootDirBuffer
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
  mov di, KernelName
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
  mov si, NoKernelMsg
  call Print
  jmp Start.Hang

.FoundKernel:
  mov ax, [si + 26]             ; first cluster
  cmp ax, 2
  jb .BadCluster

  mov [CurrentCluster], ax
  mov [SavedFirstCluster], ax

  ; file size (32-bit)
  mov ax, [si + 28]
  mov [KernelSizeLow], ax
  mov ax, [si + 30]
  mov [KernelSizeHigh], ax

  ; Compute number of sectors = ceil(size / BytesPerSector)
  ; Use DX:AX for the 32-bit size so kernels larger than 64 KB are handled.
  mov ax, [KernelSizeLow]
  mov dx, [KernelSizeHigh]
  mov bx, [BytesPerSector]      ; 512
  dec bx
  add ax, bx                    ; size + (BPS-1)
  adc dx, 0
  inc bx                        ; restore BPS
  div bx                        ; DX:AX / BX -> AX = sectors
  mov [KernelSectors], ax

  ret

.BadCluster:
  mov si, NoKernelMsg
  call Print
  jmp Start.Hang

ClusterToLBA:
  sub ax, 2                      ; (cluster - 2)
  xor dx, dx
  mov cx, [SectorsPerCluster]
  mul cx                         ; AX = (cluster-2) * SPC
  add ax, [FirstDataSector]      ; + first data sector
  ret

LoadKernel:
  ; Destination linear address (may exceed 64 KB, so track explicitly).
  ; The flat kernel image (KERNEL.QX) is already laid out with holes so that
  ; file offset 0 maps directly to physical 0x00010000. Load it exactly at
  ; KernelPMEntry to preserve the segment offsets baked into the image.
  mov dword [KernelDestLinear], KernelPMEntry

  ; Total sectors to read (already computed in FindKernel)
  mov cx, [KernelSectors]        ; CX = sectorsRemaining
  cmp cx, 0
  je .Done                       ; empty file (just in case)

  ; Start at first cluster
  mov ax, [SavedFirstCluster]
  mov si, ax                     ; SI = current cluster

.LoadCluster:
  cmp cx, 0
  je .Done                       ; all requested sectors read

  mov ax, si                     ; AX = current cluster
  cmp ax, 0x0FF8
  jae .Done                      ; EOC

  ; safety: clusters 0..1 are reserved
  cmp ax, 2
  jb .FATError

  ; convert cluster -> LBA
  push cx                        ; save sectorsRemaining
  call ClusterToLBA              ; AX = LBA
  mov dx, ax                     ; DX = LBA of first sector in this cluster
  pop cx                         ; restore sectorsRemaining

  ; sectors_this_cluster = min(SectorsPerCluster, sectorsRemaining)
  mov di, [SectorsPerCluster]    ; DI = SPC
  cmp di, cx
  jbe .SectorsOk
  mov di, cx                     ; file ends in this cluster

.SectorsOk:

.ClusterSectorLoop:
  cmp di, 0
  je .NextCluster

  ; Set ES:BX for this sector from the running linear destination pointer.
  mov eax, [KernelDestLinear]    ; EAX = dest linear
  mov bx, ax                     ; offset = low 16 bits
  and bx, 0x000F                 ; physical offset = linear & 0xF (prevent wrap)
  shr eax, 4
  mov es, ax                     ; segment = linear >> 4

  mov ax, dx                     ; AX = current LBA
  call ReadSectorLBA             ; read 1 sector -> ES:BX

  ; advance dest pointer by one sector (handle >64 KB)
  mov bx, [BytesPerSector]
  add word [KernelDestLinear], bx
  adc word [KernelDestLinear + 2], 0

  ; next LBA
  inc dx

  dec di                         ; one sector less in this cluster
  dec cx                         ; one sector less overall
  jmp .ClusterSectorLoop

.NextCluster:
  mov ax, si                     ; current cluster
  call GetNextCluster            ; AX = next
  mov si, ax
  jmp .LoadCluster

.Done:
  ret

.FATError:
  mov si, FATErrorMsg
  call Print
  jmp Start.Hang

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

.Done:
  pop si
  pop dx
  pop cx
  pop bx
  ret

ReadSectorLBA:
  push ax
  push bx
  push cx
  push dx

  ; Reset disk controller before issuing the read. Do this *before* we derive
  ; CHS, because many BIOSes clobber CH/CL/DH on reset.
  mov dl, [BootDrive]
  mov ah, 0x00
  int 0x13

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

  mov si, DiskErrorMsg
  call Print
  jmp Start.Hang

;---------------------------------------------------------------------------
; Collect E820 memory map into BootInfoPhysical.
;---------------------------------------------------------------------------
CollectE820:
  ; Ensure DS/ES point to BootInfo buffer
  xor ax, ax
  mov ds, ax
  mov ax, BootInfoPhysical >> 4
  mov es, ax

  ; Zero BootInfo buffer
  mov di, BootInfoPhysical & 0xF
  mov cx, BootInfoTotalBytes / 2
  xor ax, ax
  rep stosw

  mov dword [MemMapEntryCount], 0
  xor ebx, ebx                  ; continuation value
  mov di, (BootInfoPhysical & 0xF) + (MemMapEntriesOffset - BootInfoPhysical)

.E820Loop:
  ; Reset ES for safety (some BIOSes clobber it)
  mov ax, BootInfoPhysical >> 4
  mov es, ax

  mov eax, 0xE820
  mov edx, 0x534D4150           ; 'SMAP'
  mov ecx, MemMapEntrySize
  int 0x15
  jc .Done

  cmp eax, 0x534D4150
  jne .Done

  ; store entry already in ES:DI
  add di, MemMapEntrySize
  inc dword [MemMapEntryCount]

  cmp dword [MemMapEntryCount], MemMapMaxEntries
  jae .Done

  cmp ebx, 0
  jne .E820Loop

.Done:
  ret

EnableA20:
  ; Fast A20 (port 0x92)
  in   al, 0x92
  test al, 00000010b
  jnz  .Done
  or   al, 00000010b
  out  0x92, al

.Done:
  ret

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

ProtectedEntry:
  mov ax, DataSelector
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov fs, ax
  mov gs, ax

  mov esp, 0x90000        ; 32-bit stack (under 1MB for now)

  ; Pass BootInfo physical pointer in ESI
  mov esi, BootInfoPhysical

  ; Jump to 32-bit kernel entry
  jmp KernelPMEntry

; --------------------------------------------------------------------------
; GDT
; --------------------------------------------------------------------------

[BITS 16]

GDTStart:
  dq 0x0000000000000000       ; null
  dq 0x00CF9A000000FFFF       ; code: base=0, limit=4GB, RX
  dq 0x00CF92000000FFFF       ; data: base=0, limit=4GB, RW
GDTEnd:

GDTDescriptor:
  dw GDTEnd - GDTStart - 1
  dd GDTStart

%include "Console.inc"

times Stage2Sectors*512 - ($-$$) db 0
