/**
 * @file Kernel/KernelLog.hpp
 * @brief Declares kernel logging and mayday macros.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Handlers/IMaydayHandler.hpp"
#include "KernelContext.hpp"
#include "Logging/SpinlockLog.hpp"
#include "KernelTypes.hpp"

namespace Quantum::Kernel {
  /**
   * @brief Result of trimming a `__PRETTY_FUNCTION__` string.
   */
  struct KLogFunctionName {
    /**
     * @brief `char` pointer to the start of the trimmed function name.
     */
    const char* Start;

    /**
     * @brief Length of the trimmed function name.
     */
    int Length;
  };

  /**
   * @brief Extracts `Class::Method` from a GCC `__PRETTY_FUNCTION__` string.
   *
   * Given `"RetType Ns::Class::Method(args)"`, returns a pointer and length
   * for `"Class::Method"`. Falls back to the full string if parsing fails.
   */
  inline KLogFunctionName KLogTrimPrettyFunction(const char* pretty) {
    if (
      pretty &&
      *pretty != '\0'
    ) {
      // find the opening '(' that starts the parameter list
      const char* parenOpen = pretty;

      while (
        *parenOpen &&
        *parenOpen != '('
      ) {
        ++parenOpen;
      }

      if (parenOpen == pretty) {
        return {
          pretty,
          0
        };
      } else {
        // scan backwards past the method name to find '::'
        const char* cursor = parenOpen - 1;

        while (cursor > pretty && *cursor != ':') {
          --cursor;
        }

        // cursor is now on the second ':' of the '::' before the method name;
        // step back past '::' to find the class name
        if (
          cursor > pretty + 1 &&
          *(cursor - 1) == ':'
        ) {
          const char* classStart = cursor - 2;

          while (
            classStart > pretty &&
            *(classStart - 1) != ':' &&
            *(classStart - 1) != ' '
          ) {
            --classStart;
          }

          return {
            classStart,
            static_cast<int>(parenOpen - classStart)
          };
        } else {
          // no '::' found — free function; find the start after the return
          // type
          const char* funcStart = parenOpen - 1;

          while (
            funcStart > pretty &&
            *(funcStart - 1) != ' '
          ) {
            --funcStart;
          }

          return {
            funcStart,
            static_cast<int>(parenOpen - funcStart)
          };
        }
      }
    } else {
      return {
        "(unknown)",
        9
      };
    }
  }
}

/**
 * @brief Define `KLOG_VERBOSE` before including this header to prepend
 *        `(Class::Method line)` to every KLOG message. When not defined,
 *        messages are emitted without a source location prefix.
 */
#ifdef KLOG_VERBOSE
  #define _KLOG(level, msg, ...) \
    do { \
      auto* _klog = Context->Log; \
      if (_klog) { \
        auto _kfn = ::Quantum::Kernel::KLogTrimPrettyFunction( \
          __PRETTY_FUNCTION__ \
        ); \
        \
        _klog->level( \
          "(%.*s %d) " msg, \
          _kfn.Length, _kfn.Start, __LINE__ __VA_OPT__(,) __VA_ARGS__ \
        ); \
      } \
    } while (0)
#else
  #define _KLOG(level, msg, ...) \
    do { \
      auto* _klog = Context->Log; \
      \
      if (_klog) _klog->level(msg __VA_OPT__(,) __VA_ARGS__); \
    } while (0)
#endif

/**
 * @brief Logs a debug-level message via @ref Context.
 * @param msg Format string (printf-style).
 * @param ... Additional arguments for formatting.
 *
 * Prepends `(Class::Method:line)` to the message.
 */
#define KLOG_DEBUG(msg, ...) _KLOG(Debug, msg __VA_OPT__(,) __VA_ARGS__)

/**
 * @brief Logs a trace-level message via @ref Context.
 * @param msg Format string (printf-style).
 * @param ... Additional arguments for formatting.
 *
 * Prepends `(Class::Method:line)` to the message.
 */
#define KLOG_TRACE(msg, ...) _KLOG(Trace, msg __VA_OPT__(,) __VA_ARGS__)

/**
 * @brief Logs an info-level message via @ref Context.
 * @param msg Format string (printf-style).
 * @param ... Additional arguments for formatting.
 *
 * Prepends `(Class::Method:line)` to the message.
 */
#define KLOG_INFO(msg, ...) _KLOG(Info, msg __VA_OPT__(,) __VA_ARGS__)

/**
 * @brief Logs a warning-level message via @ref Context.
 * @param msg Format string (printf-style).
 * @param ... Additional arguments for formatting.
 *
 * Prepends `(Class::Method:line)` to the message.
 */
#define KLOG_WARNING(msg, ...) _KLOG(Warning, msg __VA_OPT__(,) __VA_ARGS__)

/**
 * @brief Logs an error-level message via @ref Context.
 * @param msg Format string (printf-style).
 * @param ... Additional arguments for formatting.
 *
 * Prepends `(Class::Method:line)` to the message.
 */
#define KLOG_ERROR(msg, ...) _KLOG(Error, msg __VA_OPT__(,) __VA_ARGS__)

/**
 * @brief Logs a critical-level message via @ref Context.
 * @param msg Format string (printf-style).
 * @param ... Additional arguments for formatting.
 *
 * Prepends `(Class::Method:line)` to the message.
 */
#define KLOG_CRITICAL(msg, ...) _KLOG(Critical, msg __VA_OPT__(,) __VA_ARGS__)

/**
 * @brief Triggers a kernel mayday via @ref Context.
 * @param msg The mayday message.
 */
#define MAYDAY(msg) \
  do { \
    auto* _mh = Context->MaydayHandler; \
    if (_mh) _mh->Handle(msg); \
    while (true) asm volatile("cli\n hlt"); \
  } while (0)
