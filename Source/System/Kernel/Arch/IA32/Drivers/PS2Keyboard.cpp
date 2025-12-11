//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Drivers/PS2Keyboard.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// PS/2 keyboard driver (basic scancode-to-ASCII and IRQ handler).
//------------------------------------------------------------------------------

#include <Arch/IA32/Drivers/IO.hpp>
#include <Arch/IA32/Drivers/PIC.hpp>
#include <Arch/IA32/Drivers/PS2Keyboard.hpp>
#include <Interrupts.hpp>
#include <Types.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  namespace {
    /**
     * Scancode to ASCII mapping for standard keys.
     */
    constexpr char scancodeMap[128] = {
      0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
      '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
      0,  'a','s','d','f','g','h','j','k','l',';','\'','`', 0,
      '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
    };

    /**
     * Scancode to ASCII mapping when Shift is active.
     */
    constexpr char scancodeMapShift[128] = {
      0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
      '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
      0,  'a','s','d','f','g','h','j','k','l',':','\"','~', 0,
      '|','z','x','c','v','b','n','m','<','>','?', 0, '*', 0, ' ',
    };

    /**
     * Left shfit make code.
     */
    constexpr UInt8 shiftLeftMake = 0x2A;

    /**
     * Right shift make code.
     */
    constexpr UInt8 shiftRightMake = 0x36;

    /**
     * Left shift break code.
     */
    constexpr UInt8 shiftLeftBreak = 0xAA;

    /**
     * Right shift break code.
     */
    constexpr UInt8 shiftRightBreak = 0xB6;

    /**
     * Control make code.
     */
    constexpr UInt8 ctrlMake = 0x1D;

    /**
     * Control break code.
     */
    constexpr UInt8 ctrlBreak = 0x9D;

    /**
     * Alt make code.
     */
    constexpr UInt8 altMake = 0x38;

    /**
     * Alt break code.
     */
    constexpr UInt8 altBreak = 0xB8;

    /**
     * Caps Lock make code.
     */
    constexpr UInt8 capsMake = 0x3A;

    /**
     * Caps Lock break code.
     */
    constexpr UInt8 capsBreak = 0xBA;

    /**
     * Keyboard input buffer size.
     */
    constexpr Size bufferSize = 64;

    /**
     * Keyboard input buffer.
     */
    char keyBuffer[bufferSize] = {};

    /**
     * Head index for the keyboard buffer.
     */
    volatile UInt8 head = 0;

    /**
     * Tail index for the keyboard buffer.
     */
    volatile UInt8 tail = 0;

    /**
     * Indicates if Shift key is active.
     */
    volatile bool shiftActive = false;

    /**
     * Indicates if Caps Lock is active.
     */
    volatile bool capsLock = false;

    /**
     * Indicates if Control key is active.
     */
    volatile bool ctrlActive = false;

    /**
     * Indicates if Alt key is active.
     */
    volatile bool altActive = false;

    /**
     * Indicates if the last scancode was an extended prefix (0xE0).
     */
    volatile bool extendedPrefix = false;

    /**
     * Enqueues a character into the keyboard buffer.
     * @param ch Character to enqueue.
     */
    inline void Enqueue(char ch) {
      UInt8 next = static_cast<UInt8>((head + 1) % bufferSize);

      if (next != tail) {
        keyBuffer[head] = ch;
        head = next;
      }
    }

    /**
     * Keyboard interrupt handler (IRQ1).
     * @param context The interrupt context.
     */
    void KeyboardHandler(InterruptContext&) {
      UInt8 scancode = IO::InByte(0x60);

      if (scancode == 0xE0) {
        // handle E0 prefix (extended scancode); mark and skip this byte
        extendedPrefix = true;
      } else if (extendedPrefix) {
        // for now, ignore extended scancodes
        extendedPrefix = false;
      } else if (scancode == shiftLeftBreak || scancode == shiftRightBreak) {
        // release handling
        shiftActive = false;
      } else if (scancode == ctrlBreak) {
        ctrlActive = false;
      } else if (scancode == altBreak) {
        altActive = false;
      } else if (scancode == capsBreak) {
        // ignore caps break
      } else if (scancode & 0x80) {
        // ignore other release events (bit 7 set)
      } else if (scancode == shiftLeftMake || scancode == shiftRightMake) {
        shiftActive = true;
      } else if (scancode == ctrlMake) {
        ctrlActive = true;
      } else if (scancode == altMake) {
        altActive = true;
      } else if (scancode == capsMake) {
        capsLock = !capsLock;
      } else if (scancode < sizeof(scancodeMap)) {
        char base = scancodeMap[scancode];
        char ch = base;

        // alphabetic keys: apply shift/caps toggling
        if (base >= 'a' && base <= 'z') {
          bool upper = (shiftActive ^ capsLock);
          ch = upper ? static_cast<char>(base - ('a' - 'A')) : base;
        } else if (shiftActive) {
          ch = scancodeMapShift[scancode];
        }

        if (ch != 0) {
          Enqueue(ch);
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

  char PS2Keyboard::ReadCharacter() {
    if (head == tail) {
      return 0;
    }
    char ch = keyBuffer[tail];
    tail = static_cast<UInt8>((tail + 1) % bufferSize);
    return ch;
  }
}
