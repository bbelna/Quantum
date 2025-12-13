; ------------------------------------------------------------------------------
; Quantum
; System/Boot/IA32/Constants.inc
; (c) 2025 Brandon Belna - MIT LIcense
; ------------------------------------------------------------------------------
; IA32 common bootloader constants.
; ------------------------------------------------------------------------------

; Stack setup (in real mode): place stack at 0x7C00 for Stage 1
%define STACK_SEGMENT 0x0000
%define STACK_OFFSET  0x7C00
