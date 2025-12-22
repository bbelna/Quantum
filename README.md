# Quantum
(c) Brandon Belna 2025 - MIT License

## Overview
Just another hobbyist operating system.
Clean, readable, and documented code is at the heart of its philosophy.
It does not try to emulate any existing operating system architectures:
it is built from the ground-up to be something new, sleek, efficient, and
modern.

How far will it go? Until I'm bored I suppose. Long-term ideas and the current
roadmap are outlined below (after Building & Debugging).

## Building & Debugging

### Windows (Ubuntu WSL)
Install the stuff you need in WSL:
```bash
sudo apt update
sudo apt install -y build-essential nasm mtools dosfstools
```

Then update `.\Tools\Build.ps1` with the project path on your Windows machine
and from within WSL.

Now you're all set up. In a normal non-WSL PowerShell, run `.\Build` to build.
If successful, a floppy disk image will be outputted to `Build\Quantum.img`.

To build a version that performs kernel testing, run `.\Build -t`.

Using the debugging script requires QEMU to be installed and your PATH set up
to point to QEMU's directory. 
To build and debug, run `.\Build -r`. To just debug, run `.\Debug`.
You can do `.\Build -r -t` to build and debug a version that runs tests.


## Planned Hardware Support
  * **Architectures**: IA32, AMD64.
  * **Storage**: CD/DVD, HDD, USB.
  * **Graphics**: As long as we have colored graphics support generally.
  * **Networking**: At least some proof of concepts.
    * Port of FreeBSD stack (a la Haiku) for fuller support?

## Vision 
* Efficient use of symmetric multiprocessing.
* Native API that focus on clean, optimized, readable code.
* Standard library support (e.g., libc) to faciliate porting
  existing applications.
* Cool, new, and fun GUI.
  * We will have a text-mode shell along the way; eventually will be
    supplanted by a GUI.
