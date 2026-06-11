/**
 * @file Clients/StreamClient.cpp
 * @brief Implements @ref @QClients::StreamClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>
#include "StreamClient.hpp"

namespace Quantum::Clients {
  bool StreamClient::CreateStream(
    SharedBufferID& outBufferID,
    UInt32 bufferSize
  ) {
    StreamCreateRequest request = {};

    request.ABIVersion = StreamABIVersion;
    request.Operation = StreamOperation::CreateStream;
    request.BufferSize = bufferSize;

    StreamCreateResult result = InvokeOS<
      StreamCreateResult,
      StreamCreateRequest
    >(StreamPortID, 0, request);

    if (!result.Success) return false;

    outBufferID = result.BufferID;

    return true;
  }

  void StreamClient::CloseWriter(SharedBufferID bufferID) {
    StreamCloseWriterRequest request = {};

    request.ABIVersion = StreamABIVersion;
    request.Operation = StreamOperation::CloseWriter;
    request.BufferID = bufferID;

    SendOS(StreamPortID, request);
  }

  void StreamClient::CloseReader(SharedBufferID bufferID) {
    StreamCloseReaderRequest request = {};

    request.ABIVersion = StreamABIVersion;
    request.Operation = StreamOperation::CloseReader;
    request.BufferID = bufferID;

    SendOS(StreamPortID, request);
  }
}
