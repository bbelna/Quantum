;-------------------------------------------------------------------------------
; Quantum
;-------------------------------------------------------------------------------
; Boot/Floppy/Floppy.asm
; Stage 2 FAT12 parser & QKRNL.QX loader. Assumes Stage 1 has loaded this sector
; at 0x0000:0x0600 (so ORG=0x0600). After finding QKRNL.QX, reads it into
; 0x0000:0x8000 and jumps there.
; Brandon Belna - MIT License
;-------------------------------------------------------------------------------

ORG 0x0600
BITS 16

%include "Constants.inc"   ; defines STACK_OFFSET, etc.
%include "Print.inc"     ; provides `Print` for DS:SI→string

Floppy:
  mov [BootDrive], dl     ; save BIOS‐drive (DL) so ReadSector can use it

  ; Read BIOS‐Parameter Block (BPB) bytes from Boot sector at 0x0000:0x7C00
  mov ax, [0x7C00 + 11]   ; BytesPerSector
  mov [BPB_BytesPerSector], ax

  mov al, [0x7C00 + 13]   ; SectorsPerCluster
  mov [BPB_SectorsPerCluster], al

  mov ax, [0x7C00 + 14]   ; ReservedSectors
  mov [BPB_ReservedSectors], ax

  mov al, [0x7C00 + 16]   ; NumFATs
  mov [BPB_NumFATs], al

  mov ax, [0x7C00 + 17]   ; MaxRootEntries
  mov [BPB_MaxRootEntries], ax

  mov ax, [0x7C00 + 22]   ; SectorsPerFAT
  mov [BPB_SectorsPerFAT], ax

  ; Scan the root directory (LBA 19..32) for “QKRNL.QX”
  xor bx, bx         ; BX = found‐flag (0=not found, 1=found)
  xor cx, cx         ; CX = sector‐index within root (0..13)

ScanRootDirSectors:
  mov ax, 19
  add ax, cx         ; AX = LBA to read this time
  call ReadSector      ; → 0x0000:0x0200

  mov si, 0x0200       ; point SI at first dir‐entry in buffer
  mov dx, 16         ; 16 entries per sector

ScanDirEntries:
  push cx
  push dx
  push si

  ; set BX to the base of this directory entry (buffer at 0x0200 + entryIndex*32)
  mov bx, si       ; BX = offset of entry within memory (e.g. 0x0200 + n*32)

  ; compare 8-byte name at [0x0000:BX] vs. KernelName
  mov di, bx       ; DI → first of 8 filename bytes
  mov si, KernelName
  call Compare8
  cmp al, 0
  jne NotThisEntry

  ; compare 3-byte extension at [0x0000:BX+8] vs. KernelExt
  mov di, bx
  add di, 8        ; DI → offset where extension begins
  mov si, KernelExt
  call Compare3
  cmp al, 0
  jne NotThisEntry

  ; at this point, we know “QKRNL” + “QX” matched

  ; restore registers
  pop si
  pop dx
  pop cx

  ; BX still holds the entry’s base (0x0200 + n*32)
  ; read starting cluster (word at offset BX+26)
  mov dx, [0x0000:bx + 26]
  mov [FoundCluster], dx

  ; read file size (dword at offset BX+28)
  mov eax, [0x0000:bx + 28]
  mov [FoundFileSize], eax

  mov bx, 1          ; mark “found”
  jmp DoneRootSearch

NotThisEntry:
  ; restore registers and move to the next entry
  pop si
  pop dx
  pop cx

  add si, 32         ; SI = next entry within this sector
  dec dx
  jnz ScanDirEntries

  ; end of this sector → move to the next root sector
  pop si
  pop dx
  pop cx
  inc cx
  cmp cx, 14
  jl ScanRootDirSectors

DoneRootSearch:
  cmp bx, 1
  je LoadKernel        ; if found, go load it

  ; If we scanned all 14 root sectors and didn’t find it:
  mov si, NotFoundMsg
  call Print
  jmp $            ; hang

;───────────────────────────────────────────────────────────────────────────
; LoadKernel
; Compute how many sectors to read and where, then read them.
;───────────────────────────────────────────────────────────────────────────
LoadKernel:
  mov eax, [FoundFileSize]
  mov [TempFileSize], eax

  ; ClustersNeeded = (FileSize + 511) / 512
  mov eax, [TempFileSize]
  add eax, 511
  mov ebx, 512
  cdq
  div ebx
  mov [ClustersNeeded], ax

  ; FirstDataSector = 33 + (FoundCluster - 2)
  mov ax, [FoundCluster]
  sub ax, 2
  add ax, 33
  mov [FirstDataSector], ax

  mov ax, [ClustersNeeded]
  mov [SectorsToRead], ax

  mov cx, [SectorsToRead]     ; CX = # of sectors to read
  mov si, [FirstDataSector]   ; SI = LBA of first data sector
  mov di, 0x8000        ; DI = destination offset (SEG=0000 → phys 0x8000)

ReadKernelLoop:
  mov ax, si          ; AX = LBA of current data sector
  call ReadSectorTo8000     ; read into 0000:DI
  add di, 512           ; advance destination by 512 bytes
  inc si            ; next LBA
  dec cx
  jnz ReadKernelLoop

  ; All sectors loaded! Jump into kernel at 0x0000:0x8000
  jmp 0x0000:0x8000

;───────────────────────────────────────────────────────────────────────────────
; ReadSector
; Read one 512-byte sector (LBA=AX) → 0x0000:0x0200, retry on error.
;───────────────────────────────────────────────────────────────────────────────
ReadSector:
  push ax
  push bx
  push cx
  push dx

  ; Convert LBA (in AX) → CHS
  mov bx, ax
  xor dx, dx
  mov ax, bx
  mov cx, 36          ; 18 sectors/track × 2 heads = 36
  div cx            ; AX=track, DX=remainder
  mov ch, al
  mov bx, dx
  mov al, 18
  div al            ; AH=head, AL=sector-1
  mov dh, ah
  mov cl, al
  inc cl

  mov dl, [BootDrive]
  mov ah, 0x02
  mov al, 1
  mov bx, 0x0200        ; buffer offset
  mov es, 0x0000

  mov byte [RetryCount], 3

.RetryRead1:
  int 0x13
  jc  .RetryFail1
  jmp .ReadDone1

.RetryFail1:
  dec byte [RetryCount]
  jnz  .RetryRead1
  mov si, ReadErrorMsg
  call Print
  jmp $

.ReadDone1:
  pop dx
  pop cx
  pop bx
  pop ax
  ret

;───────────────────────────────────────────────────────────────────────────────
; ReadSectorTo8000
; Read one sector (LBA=AX) → 0x0000:DI, retry on error.
;───────────────────────────────────────────────────────────────────────────────
ReadSectorTo8000:
  push ax
  push bx
  push cx
  push dx

  ; Convert LBA (in AX) → CHS
  mov bx, ax
  xor dx, dx
  mov ax, bx
  mov cx, 36
  div cx
  mov ch, al
  mov bx, dx
  mov al, 18
  div al
  mov dh, ah
  mov cl, al
  inc cl

  mov dl, [BootDrive]
  mov ah, 0x02
  mov al, 1
  mov bx, di          ; ES:BX → destination offset
  mov es, 0x0000

  mov byte [RetryCount], 3

.RetryRead2:
  int 0x13
  jc  .RetryFail2
  jmp .ReadDone2

.RetryFail2:
  dec byte [RetryCount]
  jnz  .RetryRead2
  mov si, ReadErrorMsg
  call Print
  jmp $

.ReadDone2:
  pop dx
  pop cx
  pop bx
  pop ax
  ret

;───────────────────────────────────────────────────────────────────────────────
; Compare8
; Compare 8 bytes at [DS:DI] vs. [DS:SI]. AL=0 if equal, ≠0 otherwise.
;───────────────────────────────────────────────────────────────────────────────
Compare8:
  mov cx, 8
.C8_Loop:
  mov al, [ds:di]
  mov ah, [ds:si]
  cmp al, ah
  jne .C8_NotEq
  inc di
  inc si
  loop .C8_Loop
  xor al, al
  ret
.C8_NotEq:
  mov al, 1
  ret

;───────────────────────────────────────────────────────────────────────────────
; Compare3
; Compare 3 bytes at [DS:DI] vs. [DS:SI]. AL=0 if equal, !=0 otherwise.
;───────────────────────────────────────────────────────────────────────────────
Compare3:
  mov cx, 3
.C3_Loop:
  mov al, [ds:di]
  mov ah, [ds:si]
  cmp al, ah
  jne .C3_NotEq
  inc di
  inc si
  loop .C3_Loop
  xor al, al
  ret
.C3_NotEq:
  mov al, 1
  ret

;───────────────────────────────────────────────────────────────────────────────
; Data & constants
;───────────────────────────────────────────────────────────────────────────────
BootDrive       db 0

BPB_BytesPerSector  dw 0
BPB_SectorsPerCluster db 0
BPB_ReservedSectors   dw 0
BPB_NumFATs       db 0
BPB_MaxRootEntries  dw 0
BPB_SectorsPerFAT   dw 0

NameBuffer      db 8 dup(0)
ExtBuffer       db 3 dup(0)
EntryOffset       dw 0

FoundCluster      dw 0
FoundFileSize     dd 0
ClustersNeeded    dw 0
FirstDataSector     dw 0
SectorsToRead     dw 0
TempFileSize      dd 0
RetryCount      db 0

KernelName  db "QKRNL   "  ; 8 bytes
KernelExt   db "QX "       ; 3 bytes

RootFoundMsg      db "Root search failed!$", 0
NotFoundMsg       db "QKRNL.QX not found.$", 0
ReadErrorMsg      db "Disk read failed!$", 0
