/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Main.hpp
 * Kernel main entry point declaration.
 */

#pragma once

#include "Types.hpp"

/**
 * Main kernel entry point.
 * @param bootInfoPhysicalAddress
 *   Physical address of the boot info block.
 */
void Main(UInt32 bootInfoPhysicalAddress);
