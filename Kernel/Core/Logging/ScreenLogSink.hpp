/**
 * @file Kernel/Logging/ScreenLogSink.hpp
 * @brief Declares @ref @QKrnl::Logging::ScreenLogSink.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Drivers/Graphics/GraphicsDriverTypes.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Logging {
  /**
   * @brief Log sink that delegates output to a graphics driver.
   *
   * Holds a pointer to the active screen device and can be redirected at
   * runtime (e.g. from VESA to S3 ViRGE) without touching any global state.
   */
  class ScreenLogSink final : public LogSink {
    public:
      /**
       * @brief Creates a new @ref ScreenLogSink with the given minimum level.
       * @param minLevel Messages below this @ref LogLevel are silently dropped.
       */
      explicit ScreenLogSink(LogLevel minLevel)
        : LogSink(
            [](char) {},
            minLevel
          )
      {
      }

      /**
       * @brief Redirects output to the given @ref GraphicsDriver.
       * @param driver Pointer to The @ref GraphicsDriver to write to, or
       *               `nullptr` to silence screen output.
       */
      void Redirect(GraphicsDriver* driver) {
        _driver = driver;
      }

      /**
       * @brief Writes a message to the active @ref GraphicsDriver.
       * @param level The @ref LogLevel of the message.
       * @param message The null-terminated message to write.
       *
       * If the message starts with a bracketed prefix (e.g. `[Proc.qx]`),
       * the prefix is extracted and rendered in gray, followed by a tab,
       * then the remaining message text in white.
       *
       * If no graphics driver has been set via @ref Redirect, or the
       * message level is below the minimum, the call is a no-op.
       */
      void Write(LogLevel level, const char* message) override {
        if (
          !_driver ||
          level < _minLevel
        ) {
          return;
        }

        if (message[0] == '[') {
          const char* closingBracket = message + 1;

          while (
            *closingBracket &&
            *closingBracket != ']'
          ) {
            closingBracket++;
          }

          if (*closingBracket == ']') {
            _driver->SetTextForegroundColor(_prefixColor);

            const char* prefixEnd = closingBracket;

            for (
              const char* dot = closingBracket - 1;
              dot > message;
              dot--
            ) {
              if (*dot == '.') {
                prefixEnd = dot;

                break;
              }
            }

            for (
              const char* cursor = message + 1;
              cursor < prefixEnd;
              cursor++
            ) {
              char buffer[2] = {
                *cursor,
                '\0'
              };

              _driver->WriteText(buffer);
            }

            _setColors(level);

            const char* body = closingBracket + 1;

            if (*body == ' ') body++;

            _driver->WriteText(" ");
            _driver->WriteText(body);
            _driver->WriteText("\n");

            return;
          }
        }

        _setColors(level);

        _driver->WriteText(message);
        _driver->WriteText("\n");
      }

      /**
       * @brief Writes a log message with an explicit length to the active
       *        graphics driver.
       * @param level The log level of the message.
       * @param length The length of the message (unused; delegates to the
       *        null-terminated overload).
       * @param message The null-terminated message to write.
       */
      void Write(
        LogLevel level,
        Size /*length*/,
        const char* message
      ) override {
        Write(
          level,
          message
        );
      }

    private:
      /**
       * @brief Color for the bracketed prefix portion of log messages.
       */
      static constexpr UInt32 _prefixColor = 0xFFA0A0A0;

      /**
       * @brief ARGB32 color for trace messages.
       */
      static constexpr UInt32 _traceColor = 0xFFAAAAAA;

      /**
       * @brief ARGB32 color for messages.
       */
      static constexpr UInt32 _messageColor = 0xFFFFFFFF;

      /**
       * @brief ARGB32 color for warning messages.
       */
      static constexpr UInt32 _warningColor = 0xFFFFFF00;

      /**
       * @brief ARGB32 color for error messages.
       */
      static constexpr UInt32 _errorColor = 0xFFFFFF00;

      /**
       * @brief ARGB32 color for critical messages.
       */
      static constexpr UInt32 _criticalColor = 0xFFFF0000;

      /**
       * @brief Pointer to the active graphics driver, or `nullptr` if screen
       *        output is silenced.
       */
      GraphicsDriver* _driver = nullptr;

      /**
       * @brief Sets the text color on the active @ref GraphicsDriver according
       *        to the given @ref LogLevel.
       * @param level The @ref LogLevel to determine the text color for.
       */
      void _setColors(LogLevel level) {
        switch (level) {
          case LogLevel::Trace: {
            _driver->SetTextForegroundColor(_traceColor);

            break;
          }

          case LogLevel::Warning: {
            _driver->SetTextForegroundColor(_warningColor);

            break;
          }

          case LogLevel::Error: {
            _driver->SetTextForegroundColor(_errorColor);

            break;
          }

          case LogLevel::Critical: {
            _driver->SetTextForegroundColor(_criticalColor);

            break;
          }

          default: {
            _driver->SetTextForegroundColor(_messageColor);

            break;
          }
        }
      }
  };
}
