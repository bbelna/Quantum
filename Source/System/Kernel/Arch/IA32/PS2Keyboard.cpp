/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/PS2Keyboard.cpp
 * IA32 PS/2 keyboard driver.
 */

#include <Arch/IA32/IO.hpp>
#include <Arch/IA32/PIC.hpp>
#include <Arch/IA32/PS2Keyboard.hpp>
#include <Arch/IA32/Types/IDT/InterruptContext.hpp>
#include <Interrupts.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using InterruptContext = Types::IDT::InterruptContext;

  namespace {
    /**
     * Scancode to ASCII mapping for standard keys.
     */
    constexpr char _scancodeMap[128] = {
      0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
      '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
      0,  'a','s','d','f','g','h','j','k','l',';','\'','`', 0,
      '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
    };

    /**
     * Scancode to ASCII mapping when Shift is active.
     */
    constexpr char _scancodeMapShift[128] = {
      0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
      '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
      0,  'a','s','d','f','g','h','j','k','l',':','\"','~', 0,
      '|','z','x','c','v','b','n','m','<','>','?', 0, '*', 0, ' ',
    };

    /**
     * Left shfit make code.
     */
    constexpr UInt8 _shiftLeftMake = 0x2A;

    /**
     * Right shift make code.
     */
    constexpr UInt8 _shiftRightMake = 0x36;

    /**
     * Left shift break code.
     */
    constexpr UInt8 _shiftLeftBreak = 0xAA;

    /**
     * Right shift break code.
     */
    constexpr UInt8 _shiftRightBreak = 0xB6;

    /**
     * Control make code.
     */
    constexpr UInt8 _ctrlMake = 0x1D;

    /**
     * Control break code.
     */
    constexpr UInt8 _ctrlBreak = 0x9D;

    /**
     * Alt make code.
     */
    constexpr UInt8 _altMake = 0x38;

    /**
     * Alt break code.
     */
    constexpr UInt8 _altBreak = 0xB8;

    /**
     * Caps Lock make code.
     */
    constexpr UInt8 _capsMake = 0x3A;

    /**
     * Caps Lock break code.
     */
    constexpr UInt8 _capsBreak = 0xBA;

    /**
     * Keyboard input buffer size.
     */
    constexpr Size _bufferSize = 64;

    /**
     * Keyboard input buffer.
     */
    char _keyBuffer[_bufferSize] = {};

    /**
     * Head index for the keyboard buffer.
     */
    volatile UInt8 _head = 0;

    /**
     * Tail index for the keyboard buffer.
     */
    volatile UInt8 _tail = 0;

    /**
     * Indicates if Shift key is active.
     */
    volatile bool _shiftActive = false;

    /**
     * Indicates if Caps Lock is active.
     */
    volatile bool _capsLock = false;

    /**
     * Indicates if Control key is active.
     */
    volatile bool _ctrlActive = false;

    /**
     * Indicates if Alt key is active.
     */
    volatile bool _altActive = false;

    /**
     * Indicates if the last scancode was an extended prefix (0xE0).
     */
    volatile bool _extendedPrefix = false;

    /**
     * Enqueues a character into the keyboard buffer.
     * @param ch
     *   Character to enqueue.
     */
    inline void Enqueue(char ch) {
      UInt8 next = static_cast<UInt8>((_head + 1) % _bufferSize);

      if (next != _tail) {
        _keyBuffer[_head] = ch;
        _head = next;
      }
    }

    /**
     * Keyboard interrupt handler (IRQ1).
     */
    void KeyboardHandler(InterruptContext&) {
      UInt8 scancode = IO::InByte(0x60);

      if (scancode == 0xE0) {
        // handle E0 prefix (extended scancode); mark and skip this byte
        _extendedPrefix = true;
      } else if (_extendedPrefix) {
        // for now, ignore extended scancodes
        _extendedPrefix = false;
      } else if (scancode == _shiftLeftBreak || scancode == _shiftRightBreak) {
        // release handling
        _shiftActive = false;
      } else if (scancode == _ctrlBreak) {
        _ctrlActive = false;
      } else if (scancode == _altBreak) {
        _altActive = false;
      } else if (scancode == _capsBreak) {
        // ignore caps break
      } else if (scancode & 0x80) {
        // ignore other release events (bit 7 set)
      } else if (scancode == _shiftLeftMake || scancode == _shiftRightMake) {
        _shiftActive = true;
      } else if (scancode == _ctrlMake) {
        _ctrlActive = true;
      } else if (scancode == _altMake) {
        _altActive = true;
      } else if (scancode == _capsMake) {
        _capsLock = !_capsLock;
      } else if (scancode < sizeof(_scancodeMap)) {
        char base = _scancodeMap[scancode];
        char ch = base;

        // alphabetic keys: apply shift/caps toggling
        if (base >= 'a' && base <= 'z') {
          bool upper = (_shiftActive ^ _capsLock);
          ch = upper ? static_cast<char>(base - ('a' - 'A')) : base;
        } else if (_shiftActive) {
          ch = _scancodeMapShift[scancode];
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
    return _head != _tail;
  }

  char PS2Keyboard::ReadCharacter() {
    if (_head == _tail) {
      return 0;
    }
    char ch = _keyBuffer[_tail];
    _tail = static_cast<UInt8>((_tail + 1) % _bufferSize);
    return ch;
  }
}
