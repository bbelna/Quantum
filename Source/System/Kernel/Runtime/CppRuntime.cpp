/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Runtime/CppRuntime.cpp
 * Minimal C++ runtime shims for freestanding kernel builds.
 */

extern "C" {
  /**
   * Thread-safe acquire guard for function-local statics.
   */
  int __cxa_guard_acquire(unsigned char* guard) {
    return *guard == 0;
  }

  /**
   * Thread-safe release guard for function-local statics.
   */
  void __cxa_guard_release(unsigned char* guard) {
    *guard = 1;
  }

  /**
   * Thread-safe abort guard for function-local statics.
   */
  void __cxa_guard_abort(unsigned char* guard) {
    *guard = 0;
  }

  /**
   * Static destructor registration is a no-op in the kernel.
   */
  int __cxa_atexit(void (*)(void*), void*, void*) {
    return 0;
  }

  /**
   * DSO handle required by some toolchains.
   */
  void* __dso_handle = nullptr;
}

/**
 * Minimal global delete operator to satisfy toolchain references. No heap
 * management is performed here.
 */
void operator delete(void* /*ptr*/, unsigned int /*size*/) noexcept {}

/**
 * Minimal global delete operator to satisfy toolchain references. No heap
 * management is performed here.
 */
void operator delete(void* /*ptr*/) noexcept {}
