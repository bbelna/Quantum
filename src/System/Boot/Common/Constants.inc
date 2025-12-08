; ------------------------------------------------------------------------------
; Quantum
; System/Boot/Common/Constants.inc
; Brandon Belna - MIT License
; ------------------------------------------------------------------------------
; Common bootloader constants.
; ------------------------------------------------------------------------------

; Stack setup (in real mode): place stack at 0x7C00 for Stage 1
%define STACK_SEGMENT 0x0000
%define STACK_OFFSET  0x7C00
