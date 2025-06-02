; ------------------------------------------------------------------------------
; Quantum
; ------------------------------------------------------------------------------
; System/Boot/Boot.asm
; Stage 1 bootloader. Load Stage 2 from sector 1 into 0x0000:0x0600, then jumps
; there.
; Brandon Belna - MIT License
; ------------------------------------------------------------------------------

[BITS 16]
[ORG 0x7C00]

%include "Constants.inc"

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

; ------------------------------------------------------------------------------
; FloppyBoot
; ------------------------------------------------------------------------------
; Bootloader for floppy disks. Reads 2 sectors starting at sector 2
; into memory at 0x0600, then jumps to that address.
; ------------------------------------------------------------------------------
FloppyBoot:
  ; Zero out AX to set DS=0 first
  xor ax, ax
  mov ds, ax

  mov ah, 0x02
  mov al, 2           ; read 2 consecutive sectors
  mov ch, 0           ; cylinder 0
  mov dh, 0           ; head 0
  mov cl, 2           ; starting at sector 2
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

%include "Print.inc"

times 510-($-$$) db 0
dw 0xAA55
