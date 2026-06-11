/**
 * @file Streaming/StandardStreams.cpp
 * @brief Implements the standard stream globals and initialization.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "StandardStreams.hpp"
#include "ProcessStreamTable.hpp"

namespace Quantum::Streaming {
  // use raw byte storage to avoid global constructors and __cxa_atexit
  // registration; Stream's default constructor is trivial (all zeroes), so
  // BSS-initialized storage is correct
  alignas(Stream) static char StandardInStorage[sizeof(Stream)];
  alignas(Stream) static char StandardOutStorage[sizeof(Stream)];
  alignas(Stream) static char StandardErrorStorage[sizeof(Stream)];

  Stream& StandardIn = reinterpret_cast<Stream&>(StandardInStorage);
  Stream& StandardOut = reinterpret_cast<Stream&>(StandardOutStorage);
  Stream& StandardError = reinterpret_cast<Stream&>(StandardErrorStorage);

  void InitializeStandardStreams() {
    const ProcessStreamTable* table
      = reinterpret_cast<const ProcessStreamTable*>(
        ProcessStreamTableAddress
      );

    // the kernel always maps the stream table page; Count is 0 when no
    // streams were inherited
    if (table->Count == 0) return;

    if (
      table->Count > StreamIndex::StandardIn &&
      table->Entries[StreamIndex::StandardIn] != 0
    ) StandardIn = Stream::Open(table->Entries[StreamIndex::StandardIn]);

    if (
      table->Count > StreamIndex::StandardOut &&
      table->Entries[StreamIndex::StandardOut] != 0
    ) StandardOut = Stream::Open(table->Entries[StreamIndex::StandardOut]);

    if (
      table->Count > StreamIndex::StandardError &&
      table->Entries[StreamIndex::StandardError] != 0
    ) StandardError = Stream::Open(table->Entries[StreamIndex::StandardError]);
  }

  void CloseStandardStreams() {
    if (StandardError.IsValid()) StandardError.Close();
    if (StandardOut.IsValid()) StandardOut.Close();
    if (StandardIn.IsValid()) StandardIn.Close();
  }
}
