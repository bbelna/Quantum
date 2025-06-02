;-------------------------------------------------------------------------------
; Quantum
;-------------------------------------------------------------------------------
; System/Boot/Floppy/Floppy.asm
; Stage-2 FAT-12 loader. Finds QKRNL.QX in the root directory, follows the
; cluster chain, copies it to 0000:8000, then jumps there.
; Brandon Belna - MIT License
;-------------------------------------------------------------------------------

[BITS 16]
[ORG 0x0600]

; ------------------------------------------------------------------------------
;  Constants
; ------------------------------------------------------------------------------
%define KERNEL_NAME   "QKRNL   "
%define KERNEL_EXT    "QX"
%define LOAD_SEG      0x0000
%define LOAD_OFF      0x8000
%define SCRATCH       0x0500        ; 512-byte temp buffer

; ------------------------------------------------------------------------------
;  Entry
; ------------------------------------------------------------------------------
Start:
  cli
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, 0x7C00
  sti

  mov [BootDrive], dl

  mov si, Stage2Msg
  call Print

  ; Re-read LBA 0 so the BPB at 0000:7C00 is valid
  xor ax, ax            ; LBA 0
  mov bx, 0x7C00
  call ReadSectorBuf
  jc  DiskFail

  ; Cache BPB fields we need
  mov al, [0x7C0D]      ; sectors/cluster
  mov [SectorsPerCluster], al
  mov ax, [0x7C0E]      ; reserved sectors
  mov [ReservedSectors], ax
  mov al, [0x7C10]      ; number of FATs
  mov [NumFATs], al
  mov ax, [0x7C11]      ; root-entry count
  mov [MaxRootEntries], ax
  mov ax, [0x7C16]      ; sectors/FAT
  mov [SectorsPerFAT], ax

  call CalcRootDirInfo   ; fills RootDirLBA, FirstDataSector, RootDirSectors

  ; Scan root directory for QKRNL.QX
  mov ax, [RootDirLBA]       ; first root-dir sector
  mov cx, [RootDirSectors]   ; sectors to scan
  mov di, SCRATCH

RootSectorLoop:
  push ax
  push cx

  mov bx, di
  call ReadSectorBuf
  jc  DiskFail

  mov si, di
  mov cx, 16                ; 16 entries / sector
DirEntryLoop:
  cmp byte [es:si], 0x00
  je   FileNotFound
  cmp byte [es:si], 0xE5
  je   NextEntry
  test byte [es:si+11], 0x08
  jnz  NextEntry

  push cx
  push si
  mov di, si
  mov cx, 11
  mov si, FileId
  repe cmpsb                ; compare 11 chars
  pop si
  pop cx
  jcxz FoundEntry

NextEntry:
  add si, 32
  loop DirEntryLoop

  pop cx
  pop ax
  inc ax
  loop RootSectorLoop
  jmp FileNotFound

FoundEntry:
  mov ax, [es:si+26]        ; first-cluster
  mov [NextCluster], ax
  pop cx
  pop ax                    ; clear saved regs

  ; Load the kernel cluster-by-cluster
  mov ax, LOAD_SEG
  mov es, ax
  mov di, LOAD_OFF

LoadClusterLoop:
  mov ax, [NextCluster]
  cmp ax, 0x0FF8
  jae  KernelDone

  ; first sector of this cluster
  mov bx, ax                ; BX = cluster
  sub bx, 2                 ; index within data area
  mov ax, bx
  xor cx, cx
  mov cl, [SectorsPerCluster]
  mul cx                    ; DX:AX = index * spc
  add ax, [FirstDataSector] ; AX = LBA of first sector

  ; load every sector in the cluster
  mov al, [SectorsPerCluster]
  cbw
  mov si, ax                ; SI = sectors left
ClusterSectorLoop:
  push ax
  push si

  mov bx, di
  call ReadSectorBuf
  jc  DiskFail

  ; advance ES:DI by 512 bytes
  add di, 512
  jnc PtrOK
  push ax
  mov ax, es
  add ax, 0x20              ; +32 paragraphs = 512 bytes
  mov es, ax
  pop ax
PtrOK:
  pop si
  pop ax
  inc ax
  dec si
  jnz ClusterSectorLoop

  ; next cluster from FAT
  mov ax, [NextCluster]
  call GetNextCluster
  mov [NextCluster], ax
  jmp LoadClusterLoop

; ------------------------------------------------------------------------------
; KernelDone
; ------------------------------------------------------------------------------
; Jumps to the loaded kernel at 0000:8000.
; ------------------------------------------------------------------------------
KernelDone:
  cli
  xor ax, ax
  mov ds, ax
  mov es, ax
  push word LOAD_SEG
  push word LOAD_OFF        ; stack = SEG,OFF
  retf                      ; far return → kernel

; ------------------------------------------------------------------------------
; ReadSectorBuf
; ------------------------------------------------------------------------------
; Input: AX=LBA, ES:BX=dest, DL=[BootDrive]
; Reads one 512-byte sector.
; ------------------------------------------------------------------------------
ReadSectorBuf:
  push ax
  push bx
  push cx
  push dx
  push si

  ; LBA → CHS (18 spt, 2 heads)
  xor dx, dx
  mov cx, 36
  div cx                    ; AX=cyl, DX=rem (0-35)
  mov ch, al                ; cyl low
  mov ax, dx
  xor dx, dx
  mov bl, 18
  div bl                    ; AL=head, AH=sec-1
  mov dh, al
  mov cl, ah
  inc cl

  mov dl, [BootDrive]
  mov ah, 0x02
  mov al, 1
  int 0x13
  jc  .fail

  pop si
  pop dx
  pop cx
  pop bx
  pop ax
  clc
  ret
.fail:
  pop si
  pop dx
  pop cx
  pop bx
  pop ax
  stc
  ret

; ------------------------------------------------------------------------------
; CalcRootDirInfo
; ------------------------------------------------------------------------------
; Fills RootDirLBA, FirstDataSector, RootDirSectors
; ------------------------------------------------------------------------------
CalcRootDirInfo:
  mov ax, [MaxRootEntries]
  shl ax, 5                 ; *32 bytes/entry
  add ax, 511
  shr ax, 9                 ; /512
  mov [RootDirSectors], ax  ; (=14 on 1.44 MB floppy)

  mov ax, [SectorsPerFAT]
  mov bl, [NumFATs]
  xor bh, bh
  mul bx                    ; DX:AX = nFATs*spf
  add ax, [ReservedSectors]
  mov [RootDirLBA], ax

  add ax, [RootDirSectors]
  mov [FirstDataSector], ax
  ret

; ------------------------------------------------------------------------------
; GetNextCluster
; ------------------------------------------------------------------------------
; Input: AX = current cluster
; Output: AX = next cluster (0xFF8..=EOC)
; ------------------------------------------------------------------------------
GetNextCluster:
  push bx
  push cx
  push dx
  push si
  push di
  push es

  mov di, ax                ; keep cluster for parity
  mov cx, ax
  shr cx, 1
  add cx, ax                ; offset = c + c/2

  mov ax, cx
  mov cl, 9
  shr ax, cl                ; sector index
  mov dx, cx
  and dx, 511               ; byte offset within sector

  add ax, [ReservedSectors] ; LBA of FAT sector

  ; read FAT sector into 0000:SCRATCH
  mov si, ax                ; save LBA
  xor ax, ax
  mov es, ax
  mov ax, si
  mov bx, SCRATCH
  call ReadSectorBuf
  jc  DiskFail

  mov si, SCRATCH
  add si, dx
  mov ax, [es:si]

  test di, 1
  jz  .even
  shr ax, 4
  jmp .done
.even:
  and ax, 0x0FFF
.done:
  pop es
  pop di
  pop si
  pop dx
  pop cx
  pop bx
  ret

; ------------------------------------------------------------------------------
;  Error handlers
; ------------------------------------------------------------------------------
DiskFail:
  mov si, DiskErrMsg
  call Print
  jmp $

FileNotFound:
  mov si, NotFoundMsg
  call Print
  jmp $

BadDisk:
  mov si, BadDiskMsg
  call Print
  jmp $

%include "Print.inc"

; ------------------------------------------------------------------------------
;  Data
; ------------------------------------------------------------------------------
Stage2Msg           db "Stage 2",13,10,0
FileId              db KERNEL_NAME, KERNEL_EXT,0

DiskErrMsg          db 13,10,"Disk read failed!",13,10,0
NotFoundMsg         db 13,10,"QKRNL.QX not found!",13,10,0
BadDiskMsg          db 13,10,"Invalid BPB!",13,10,0

; BPB cache & derived values
SectorsPerCluster   db 0
ReservedSectors     dw 0
NumFATs             db 0
MaxRootEntries      dw 0
SectorsPerFAT       dw 0
RootDirSectors      dw 0
RootDirLBA          dw 0
FirstDataSector     dw 0

; State
BootDrive           db 0
NextCluster         dw 0
KernelSize          dw 0        ; (not yet used)

times 1024-($-$$) db 0
