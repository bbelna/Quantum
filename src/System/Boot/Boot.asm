;------------------------------------------------------------------------------
; Quantum
;------------------------------------------------------------------------------
; Boot/Boot.asm
; Stage 1 bootloader. Load Stage 2 from sector 1 into 0x0000:0x0600, then jumps
; there.
; Brandon Belna - MIT License
;------------------------------------------------------------------------------

ORG 0x0600
BITS 16

%include "Common/Constants.inc"
%include "Common/Print.inc"

Boot:
  ; Basic segment setup & stack
  cli
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, STACK_OFFSET
  sti

  ; Determine boot medium and jump to appropriate bootloader
  cmp dl, 0x00        ; DL==0x00 → Floppy A:
  je FloppyBoot
  cmp dl, 0x01        ; DL==0x01 → Floppy B:
  je FloppyBoot
  cmp dl, 0x80        ; DL>=0x80 → HDD
  jb Unknown

FloppyBoot:
  mov ah, 0x02         ; INT 13h - Read Sectors
  mov al, 1            ; read 1 sector
  mov ch, 0            ; cylinder 0
  mov dh, 0            ; head 0
  mov cl, 2            ; sector 2  (LBA 1 on a 1.44 MB floppy)
  xor bx, bx
  mov bx, 0x0600       ; offset 0x0600 into segment ES
  xor ax, ax
  mov es, ax           ; ES = 0x0000
  int 0x13             ; DL already contains the boot-drive number
  jc DiskError
  jmp 0x0000:0x0600    ; Jump to stage 2

Unknown:
  mov si, UnknownMsg
  call Print
  hlt

UnknownMsg db "Unknown boot medium!$", 0

times 510-($-$$) db 0
dw 0xAA55
