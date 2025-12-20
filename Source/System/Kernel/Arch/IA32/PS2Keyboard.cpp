/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/PS2Keyboard.cpp
 * IA32 PS/2 keyboard driver.
 */

#include <Interrupts.hpp>
#include <Arch/IA32/IO.hpp>
#include <Arch/IA32/PIC.hpp>
#include <Arch/IA32/PS2Keyboard.hpp>
#include <Arch/IA32/Interrupts.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  const char PS2Keyboard::_scancodeMap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`', 0,
    '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
  };

  const char PS2Keyboard::_scancodeMapShift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',':','\"','~', 0,
    '|','z','x','c','v','b','n','m','<','>','?', 0, '*', 0, ' ',
  };

  char PS2Keyboard::_keyBuffer[PS2Keyboard::_bufferSize] = {};
  volatile UInt8 PS2Keyboard::_head = 0;
  volatile UInt8 PS2Keyboard::_tail = 0;
  volatile bool PS2Keyboard::_shiftActive = false;
  volatile bool PS2Keyboard::_capsLock = false;
  volatile bool PS2Keyboard::_ctrlActive = false;
  volatile bool PS2Keyboard::_altActive = false;
  volatile bool PS2Keyboard::_extendedPrefix = false;

  void PS2Keyboard::Enqueue(char ch) {
    UInt8 next = static_cast<UInt8>((_head + 1) % _bufferSize);

    if (next != _tail) {
      _keyBuffer[_head] = ch;
      _head = next;
    }
  }

  Interrupts::Context* PS2Keyboard::KeyboardHandler(
    Interrupts::Context& context
  ) {
    UInt8 scancode = IO::In8(0x60);

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

    return &context;
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
