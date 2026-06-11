/**
 * @file Kernel/Arch/IA32/Interrupts/IA32InterruptConstants.hpp
 * @brief Declares constants for @ref @QKrnlIA32
 *        @ref @QKrnlIA32::Interrupts.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

/**
 * @brief End-of-interrupt command code for the PIC.
 */
#define PIC_EOI 0x20

/**
 * @brief I/O port addresses for the PIC 1 command port.
 */
#define PIC1_COMMAND_PORT 0x20

/**
 * @brief I/O port address for the PIC 1 data port.
 */
#define PIC1_DATA_PORT 0x21

/**
 * @brief I/O port address for the PIC 2 command port.
 */
#define PIC2_COMMAND_PORT 0xA0

/**
 * @brief I/O port address for the PIC 2 data port.
 */
#define PIC2_DATA_PORT 0xA1

/**
 * @brief Initialization control word 1 (ICW1): begin initialization sequence.
 *
 * Bit 4 of ICW1 must be set to signal that the PIC should expect ICW2-ICW4
 * on subsequent writes to the data port.
 */
#define PIC_ICW1_INIT 0x10

/**
 * @brief ICW1 flag indicating that ICW4 will be sent.
 *
 * When set in ICW1, the PIC expects a fourth initialization word (ICW4)
 * on the data port after ICW3.
 */
#define PIC_ICW1_ICW4 0x01

/**
 * @brief ICW4 flag: 8086/88 mode (as opposed to MCS-80/85 mode).
 */
#define PIC_ICW4_8086 0x01

/**
 * @brief Base interrupt vector for IRQs.
 */
#define IRQ_BASE_VECTOR 0x20

/**
 * @brief Number of CPU exceptions (ISRs).
 */
#define ISR_EXCEPTION_COUNT 32

/**
 * @brief Number of IRQs.
 */
#define IRQ_COUNT 16

/**
 * @brief Total number of IDT entries.
 */
#define IDT_ENTRY_COUNT 256

/**
 * @brief Interrupt vector for divide-by-zero faults.
 */
#define IRQ_VECTOR_DIVIDE_BY_ZERO 0

/**
 * @brief Interrupt vector for invalid opcode faults.
 */
#define IRQ_VECTOR_INVALID_OPCODE 6

/**
 * @brief Interrupt vector for general protection faults.
 */
#define IRQ_VECTOR_GENERAL_PROTECTION_FAULT 13

/**
 * @brief Interrupt vector for page fault exceptions.
 */
#define IRQ_VECTOR_PAGE_FAULT 14

/**
 * @brief Interrupt vector for timer IRQs.
 */
#define IRQ_VECTOR_TIMER 32

/**
 * @brief Interrupt vector for software-initiated scheduler yields.
 *
 * Outside the hardware IRQ range (32-47), so Dispatch will not send
 * a spurious EOI and the timer tick count will not be incremented.
 */
#define IRQ_VECTOR_YIELD 49

/**
 * @brief Interrupt vector for system calls.
 */
#define IRQ_VECTOR_SYSTEM_CALL 128

/**
 * @brief IRQ number for the system timer.
 */
#define IRQ_TIMER 0
