//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Drivers/PS2Keyboard.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// PS/2 keyboard driver (basic scancode-to-ASCII and IRQ handler).
//------------------------------------------------------------------------------

#include <Interrupts.hpp>
#include <Types.hpp>
#include <Drivers/Console.hpp>
#include <Arch/IA32/Drivers/IO.hpp>
#include <Arch/IA32/Drivers/PS2Keyboard.hpp>
#include <Arch/IA32/Drivers/PIC.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  namespace {
    constexpr char scancodeMap[128] = {
      0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
      '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
      0,  'a','s','d','f','g','h','j','k','l',';','\'','`', 0,
      '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
    };

    constexpr char scancodeMapShift[128] = {
      0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
      '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', // letters handled separately
      0,  'a','s','d','f','g','h','j','k','l',':','\"','~', 0,
      '|','z','x','c','v','b','n','m','<','>','?', 0, '*', 0, ' ',
    };

    constexpr UInt8 shiftLeftMake   = 0x2A;
    constexpr UInt8 shiftRightMake  = 0x36;
    constexpr UInt8 shiftLeftBreak  = 0xAA;
    constexpr UInt8 shiftRightBreak = 0xB6;

    constexpr UInt8 ctrlMake  = 0x1D;
    constexpr UInt8 ctrlBreak = 0x9D;
    constexpr UInt8 altMake   = 0x38;
    constexpr UInt8 altBreak  = 0xB8;
    constexpr UInt8 capsMake  = 0x3A;
    constexpr UInt8 capsBreak = 0xBA;

    constexpr Size bufferSize = 64;
    char keyBuffer[bufferSize] = {};
    volatile UInt8 head = 0;
    volatile UInt8 tail = 0;
    volatile bool shiftActive = false;
    volatile bool capsLock = false;
    volatile bool ctrlActive = false;
    volatile bool altActive = false;
    volatile bool extendedPrefix = false;
    volatile bool echoEnabled = false;

    inline void Enqueue(char ch) {
      UInt8 next = static_cast<UInt8>((head + 1) % bufferSize);
      if (next != tail) {
        keyBuffer[head] = ch;
        head = next;
      }
    }

    /**
     * Keyboard interrupt handler (IRQ1).
     */
    void KeyboardHandler(InterruptContext&) {
      UInt8 scancode = IO::InByte(0x60);

      // Handle E0 prefix (extended scancode); mark and skip this byte.
      if (scancode == 0xE0) {
        extendedPrefix = true;
        return;
      }

      // For now, ignore extended scancodes.
      if (extendedPrefix) {
        extendedPrefix = false;
        return;
      }

      // Release handling
      if (scancode == shiftLeftBreak || scancode == shiftRightBreak) {
        shiftActive = false;
        return;
      }
      if (scancode == ctrlBreak) {
        ctrlActive = false;
        return;
      }
      if (scancode == altBreak) {
        altActive = false;
        return;
      }
      if (scancode == capsBreak) {
        // ignore caps break
        return;
      }

      // Ignore other release events (bit 7 set).
      if (scancode & 0x80) {
        return;
      }

      if (scancode == shiftLeftMake || scancode == shiftRightMake) {
        shiftActive = true;
        return;
      }
      if (scancode == ctrlMake) {
        ctrlActive = true;
        return;
      }
      if (scancode == altMake) {
        altActive = true;
        return;
      }
      if (scancode == capsMake) {
        capsLock = !capsLock;
        return;
      }

      if (scancode < sizeof(scancodeMap)) {
        char base = scancodeMap[scancode];
        char ch = base;

        // Alphabetic keys: apply shift/caps toggling
        if (base >= 'a' && base <= 'z') {
          bool upper = (shiftActive ^ capsLock);
          ch = upper ? static_cast<char>(base - ('a' - 'A')) : base;
        } else if (shiftActive) {
          ch = scancodeMapShift[scancode];
        }

        if (ch != 0) {
          Enqueue(ch);
          if (echoEnabled) {
            Quantum::Kernel::Drivers::Console::WriteCharacter(ch);
          }
        }
      }
    }
  }

  void PS2Keyboard::Initialize() {
    Interrupts::RegisterHandler(33, KeyboardHandler); // IRQ1 vector
    PIC::Unmask(1);
  }

  bool PS2Keyboard::KeyAvailable() {
    return head != tail;
  }

  char PS2Keyboard::ReadChar() {
    if (head == tail) {
      return 0;
    }
    char ch = keyBuffer[tail];
    tail = static_cast<UInt8>((tail + 1) % bufferSize);
    return ch;
  }

  void PS2Keyboard::SetEchoEnabled(bool enabled) {
    echoEnabled = enabled;
  }
}
