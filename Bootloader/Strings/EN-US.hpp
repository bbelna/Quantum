/**
 * @file Bootloader/Strings/EN-US.hpp
 * @brief Defines US English strings for the bootloader environment.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Version.hpp>

/**
 * @brief Legal notice shown in the bootloader shell (US English).
 */
#define STRINGS_LEGAL \
  "Copyright (c) 2025-2026 The Quantum Software Project.\n" \
  "All rights reserved.\n\n" \
  \
  "Quantum is free software licensed under the GNU General Public\n" \
  "License (GPL) v2.0-only and comes with ABSOLUTELY NO WARRANTY.\n" \
  "A copy of the license should have been provided with QuantumOS.\n" \
  "The license text is available at:\n" \
  "  https://spdx.org/licenses/GPL-2.0-only.html\n\n"

/**
 * @brief Welcome message shown on boot (US English).
 */
#define STRINGS_WELCOME \
  "Welcome to Quantum " QUANTUMOS_RELEASE "!\n" \
  STRINGS_LEGAL \
  \
  "Press any key for boot shell."

/**
 * @brief Message shown when the bootloader is entering the interactive shell
 *        (US English).
 */
#define STRINGS_ENTERING_SHELL "Entering boot shell...\n"

/**
 * @brief Message shown when the bootloader is missing a concrete
 *        @ref IFileLoader implementation (US English).
 */
#define STRINGS_ERROR_NO_FILE_LOADER "No file loader available\n"

/**
 * @brief First part of the prompt shown when an unknown command is entered in
 *        the bootloader shell (US English).
 */
#define STRINGS_SHELL_UNKNOWN_COMMAND_PT1 "Unknown command \""

/**
 * @brief Second part of the prompt shown when an unknown command is entered in
 *        the bootloader shell (US English).
 */
#define STRINGS_SHELL_UNKNOWN_COMMAND_PT2 \
  "\". Enter \"?\" for help or \"boot\" to continue.\n"

/**
 * @brief Prompt shown after the intro and legal notice in the bootloader shell
 *        (US English).
 */
#define STRINGS_SHELL_HELP_PROMPT \
  "Enter \"?\" for help or \"boot\" to continue.\n"

/**
 * @brief Message shown when the bootloader is loading the kernel (US English).
 */
#define STRINGS_LOADING_KERNEL "Loading kernel... "

/**
 * @brief Message shown when the bootloader is loading the initial image
 *        (US English).
 */
#define STRINGS_LOADING_INITIAL_IMAGE "Loading initial image... "

/**
 * @brief Prefix for error messages shown in the bootloader, e.g. load failures
 *        (US English).
 */
#define STRINGS_ERROR_PREFIX "ERROR: "

/**
 * @brief First part of the message shown when the bootloader encounters an
 *        error loading a file (US English).
 */
#define STRINGS_LOAD_ERROR_PT1 "Could not load file \""

/**
 * @brief Second part of the message shown when the bootloader encounters an
 *        error loading a file (US English).
 */
#define STRINGS_LOAD_ERROR_PT2 "\"."

/**
 * @brief Prompt shown in the bootloader shell (US English).
 */
#define STRINGS_SHELL_PROMPT ">"
