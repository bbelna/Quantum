;------------------------------------------------------------------------------
; Quantum
;------------------------------------------------------------------------------
; Boot/Floppy/Floppy.asm
; Stage-2 loader. Locates QKRNL.QX on a FAT12 floppy, load it to 0x0000:0x8000,
; then jumps. Assumes Stage-1 placed this file at 0x0000:0x0600.
; Brandon Belna - MIT License
;------------------------------------------------------------------------------

ORG 0x0600
BITS 16

%include "Constants.inc"     ; STACK_OFFSET, etc.

; ─────────────────────────────────────────────────────────────────────────────
; Banner -- proves we executed Stage-2
; ─────────────────────────────────────────────────────────────────────────────
Floppy:
  mov si, Stage2Msg
  call Print

  mov [BootDrive], dl      ; remember BIOS drive #

; ─────────────────────────────────────────────────────────────────────────────
; 1.  Read BPB from boot sector (0x7C00) into variables
; ─────────────────────────────────────────────────────────────────────────────
  mov ax, [0x7C00+11]      ; BytesPerSector
  mov [BPB_BytesPerSector], ax

  mov al, [0x7C00+13]      ; SectorsPerCluster
  mov [BPB_SectorsPerCluster], al

  mov ax, [0x7C00+14]      ; ReservedSectors
  mov [BPB_ReservedSectors], ax

  mov al, [0x7C00+16]      ; NumFATs
  mov [BPB_NumFATs], al

  mov ax, [0x7C00+17]      ; MaxRootEntries
  mov [BPB_MaxRootEntries], ax

  mov ax, [0x7C00+22]      ; SectorsPerFAT
  mov [BPB_SectorsPerFAT], ax

; ─────────────────────────────────────────────────────────────────────────────
; 2.  Derived layout values
; ─────────────────────────────────────────────────────────────────────────────
  ; FirstRootLBA = ReservedSectors + NumFATs * SectorsPerFAT
  mov al, [BPB_NumFATs]    ; AL = NumFATs (<=2)
  mov ah, 0
  mul word [BPB_SectorsPerFAT] ; AX = NumFATs * SectorsPerFAT
  add ax, [BPB_ReservedSectors]
  mov [FirstRootLBA], ax

  ; RootDirSectors = (MaxRootEntries * 32) / BytesPerSector
  mov ax, [BPB_MaxRootEntries] ; 224
  shl ax, 5          ; ×32  -> 7168
  xor dx, dx
  mov bx, [BPB_BytesPerSector] ; 512
  div bx             ; AX = 14
  mov [RootDirSectors], ax

  ; FirstDataSector = FirstRootLBA + RootDirSectors
  mov ax, [FirstRootLBA]
  add ax, [RootDirSectors]
  mov [FirstDataSector], ax

; ─────────────────────────────────────────────────────────────────────────────
; 3.  Load FAT-1 (SectorsPerFAT sectors) to 0x0000:0x0400
; ─────────────────────────────────────────────────────────────────────────────
  mov si, [BPB_ReservedSectors]   ; LBA of FAT-1
  mov cx, [BPB_SectorsPerFAT]
  mov di, 0x0400          ; buffer
LoadFAT:
  mov ax, si            ; AX = current LBA
  xor dx, dx
  mov es, dx            ; ES = 0
  mov bx, di            ; ES:BX dest
  call ReadSectorBuf
  add di, 512
  inc si
  loop LoadFAT

; ─────────────────────────────────────────────────────────────────────────────
; 4.  Scan root directory for  "QKRNL   QX "
; ─────────────────────────────────────────────────────────────────────────────
  mov ax, [FirstRootLBA]
  mov [CurrRootLBA], ax
  mov cx, [RootDirSectors]

NextRootSector:
  mov ax, [CurrRootLBA]
  xor dx, dx
  mov es, dx
  mov bx, 0x0200
  call ReadSectorBuf

  mov di, 0x0200
  mov dx, 16            ; 16 entries in this sector

NextEntry:
  push dx
  push di

  ; compare 8-byte name
  mov si, KernelName
  mov cx, 8
  call Compare8
  or  al, al
  jnz NotMatch

  ; compare 3-byte extension
  add di, 8
  mov si, KernelExt
  mov cx, 3
  call Compare3
  or  al, al
  jnz NotMatch

  ; ---- MATCH! ----
  pop di
  pop dx

  mov ax, [di+26]         ; first cluster
  mov [FoundCluster], ax
  mov ax, [di+28]         ; size low
  mov dx, [di+30]         ; size high
  mov [FoundFileSize], ax
  mov [FoundFileSize+2], dx
  jmp LoadKernel

NotMatch:
  pop di
  pop dx
  add di, 32
  dec dx
  jnz NextEntry

  ; next root sector
  inc word [CurrRootLBA]
  dec cx
  jnz NextRootSector

  ; not found
  mov si, NotFoundMsg
  call Print
  jmp $

; ─────────────────────────────────────────────────────────────────────────────
; 5.  Follow FAT chain, load clusters into 0x0000:0x8000
; ─────────────────────────────────────────────────────────────────────────────
LoadKernel:
  ; sectorsRemaining = ceil(filesize/512)
  mov ax, [FoundFileSize]
  add ax, 511
  shr ax, 9
  mov [SectorsRemain], ax

  mov ax, [FoundCluster]
  mov [CurrCluster], ax
  mov di, 0x8000          ; destination pointer

LoadCluster:
  ; sectorLBA = FirstDataSector + (cluster-2)*SPC
  mov ax, [CurrCluster]
  sub ax, 2
  xor dx, dx
  mov bx, [BPB_SectorsPerCluster]
  mul bx
  add ax, [FirstDataSector]
  mov [CurrClusterLBA], ax

  ; read SPC sectors (usually 1)
  mov cx, [BPB_SectorsPerCluster]
  mov dx, ax            ; DX = first LBA of cluster

ReadSecInClus:
  mov ax, dx
  xor dx, dx
  mov es, dx
  mov bx, di
  call ReadSectorBuf
  add di, 512
  inc ax
  mov dx, ax
  dec cx
  jnz ReadSecInClus

  dec word [SectorsRemain]
  jz KernelLoaded

  ; ---- next cluster via FAT12 ----
  mov ax, [CurrCluster]
  call GetNextCluster
  mov [CurrCluster], ax
  cmp ax, 0xFF8
  jb LoadCluster

DiskFail:
  mov si, ReadErrMsg
  call Print
  jmp $

KernelLoaded:
  jmp 0x0000:0x8000

; ─────────────────────────────────────────────────────────────────────────────
; ReadSectorBuf  (LBA in AX, ES:BX dest)  - retry 3x
; ─────────────────────────────────────────────────────────────────────────────
ReadSectorBuf:
  push ax
  push bx
  push cx
  push dx

  xor  dx, dx      ; DX:AX = LBA   (high word = 0)
  mov  cx, 36      ; sectors per cylinder (18 × 2)
  div  cx        ; AX = cylinder, DX = temp (head*18 + sector-1)

  mov  ch, al      ; CH = cylinder (low 8 bits)
  xor  ax, ax
  mov  ax, dx      ; AX = temp
  mov  cl, 18
  div  cl        ; AL = head, AH = sector-1
  mov  dh, al      ; DH = head (0 or 1)
  mov  cl, ah
  inc  cl        ; CL = sector (1-18)

  mov  dl, [BootDrive]
  mov  ah, 02h       ; INT 13h - read
  mov  al, 1       ; 1 sector

.retry:
  int  13h
  jnc  .done
  dec  byte [Retry]
  jnz  .retry
  jmp  DiskFail

.done:
  pop  dx
  pop  cx
  pop  bx
  pop  ax
  ret

; ─────────────────────────────────────────────────────────────────────────────
; GetNextCluster (FAT12) - uses FAT copy at 0x0400
;  IN:  AX = current cluster (N)   OUT: AX = next cluster
; ─────────────────────────────────────────────────────────────────────────────
GetNextCluster:
  push bx
  push cx
  push dx
  push si

  mov bx, ax
  shl bx, 1
  add bx, ax        ; BX = 3*N
  shr bx, 1         ; BX = floor((3*N)/2)

  mov si, 0x0400
  add si, bx
  mov cx, [ds:si]     ; two bytes

  test ax, 1        ; even or odd cluster?
  jz  .even
  shr cx, 4         ; odd: high 12 bits
  jmp .done
.even:
  and cx, 0x0FFF      ; even: low 12 bits
.done:
  mov ax, cx
  pop si
  pop dx
  pop cx
  pop bx
  ret

; ─────────────────────────────────────────────────────────────────────────────
; Compare helpers (DS:DI vs DS:SI - AL=0 if equal)
; ─────────────────────────────────────────────────────────────────────────────
Compare8: 
  mov cx,8
.c8:
  lodsb
  scasb
  jne .neq
  loop .c8
  xor al,al
  ret
.neq:
  mov al,1
  ret

Compare3:
  mov cx,3
.c3:
  lodsb
  scasb
  jne .neq3
  loop .c3
  xor al,al
  ret
.neq3:
  mov al,1
  ret

; ─────────────────────────────────────────────────────────────────────────────
; Data
; ─────────────────────────────────────────────────────────────────────────────
BootDrive       db 0

BPB_BytesPerSector    dw 0
BPB_SectorsPerCluster   db 0
BPB_ReservedSectors   dw 0
BPB_NumFATs       db 0
BPB_MaxRootEntries    dw 0
BPB_SectorsPerFAT     dw 0

FirstRootLBA    dw 0
RootDirSectors    dw 0
FirstDataSector   dw 0
CurrRootLBA     dw 0

FoundCluster    dw 0
FoundFileSize     dd 0
CurrCluster     dw 0
CurrClusterLBA    dw 0
SectorsRemain     dw 0
Retry         db 0

KernelName  db "QKRNL   "
KernelExt   db "QX "

Stage2Msg   db "Stage 2",0
NotFoundMsg db "QKRNL.QX not found.",0
ReadErrMsg  db "Disk read failed!",0

%include "Print.inc"       ; keeps Print after loader code

times 1024-($-$$) db 0
