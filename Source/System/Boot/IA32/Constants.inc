;-------------------------------------------------------------------------------
; Quantum
;-------------------------------------------------------------------------------
; File: System/Boot/IA32/Constants.inc
; Brief: IA32 common bootloader constants.
; Author: Brandon Belna <bbelna@aol.com>
; Copyright: © 2025-2026 The Quantum OS Project
; License: GPL 2.0
;-------------------------------------------------------------------------------

; Stack setup (in real mode): place stack at 0x7C00 for Stage 1
%define STACK_SEGMENT 0x0000
%define STACK_OFFSET  0x7C00
