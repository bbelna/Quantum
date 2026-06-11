/**
 * @file Include/Quantum/Core/Result.hpp
 * @brief Declaration of result types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

namespace Quantum::Core {
  /**
   * @brief Represents the result of an operation, containing success status
   *        and associated data.
   * @tparam DataType The type of the result data.
   */
  template <typename DataType = void*>
  struct Result {
    /**
     * @brief Constructs a `Result` with the given data.
     * @param success Indicates whether the result is successful.
     * @param data The result data.
     */
    Result(bool success, DataType data)
      : Success(success), Data(data) {}

    /**
     * @brief Constructs a `Result` without data.
     * @param success Indicates whether the result is successful.
     */
    Result(bool success) : Success(success), Data() {}

    /**
     * @brief Indicates whether the result is successful.
     */
    bool Success;

    /**
     * @brief The result data.
     */
    DataType Data;
  };

  /**
   * @brief Extends @ref Result to include a count.
   * @tparam DataType The type of the result data.
   */
  template <typename DataType = void*>
  struct ResultWithCount : public Result<DataType> {
    /**
     * @brief Constructs a `ResultWithCount` with the given data and count.
     * @param success Indicates whether the result is successful.
     * @param data The result data.
     * @param count The count associated with the result.
     */
    ResultWithCount(bool success, DataType data, Size count)
      : Result<DataType>(success, data), Count(count) {}

    /**
     * @brief Constructs a `ResultWithCount` without data.
     * @param success Indicates whether the result is successful.
     * @param count The count associated with the result.
     */
    ResultWithCount() : Result<DataType>(false), Count(0) {}

    /**
     * @brief The count associated with the result.
     */
    Size Count;
  };
}

namespace Quantum {
  using namespace Core;
}
