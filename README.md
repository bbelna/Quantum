# Quantum
(c) Brandon Belna 2025 - MIT License

## Overview
Just another hobbyist operating system.
Clean, readable, and documented code is at the heart of its philosophy.
It does not try to emulate any existing operating system architectures:
it is built from the ground-up to be something new, sleek, efficient, and
modern.

How far will it go? Until I'm bored I suppose.

## Building & Debugging

### Windows (Ubuntu WSL)
Install the stuff you need in WSL:
```bash
sudo apt update
sudo apt install -y build-essential nasm mtools dosfstools
```

Then update `.\Tools\Build.ps1` with the project path on your Windows machine
and from within WSL (hopefully will develop something better like a config file
or something for the paths).

Now you're all set up. In a normal non-WSL PowerShell, run `.\Build` to build.
If successful, a floppy disk image will be outputted to `Build\Quantum.img`.

To build a version that performs kernel testing, run `.\Build -t`.

Using the debugging script requires QEMU to be installed and your PATH set up
to point to QEMU's directory. 
To build and debug, run `.\Build -r`. To just debug, run `.\Debug`.
You can do `.\Build -r -t` to build and debug a version that runs tests.

## Contributing

Issues is where all tasks live. Feel free to take an unassigned task and create
a PR. Or file your own issue if you find/think of something we need to do.

There currently is no documentation that details Quantum's code style,
architecture, etc. I will create some in the future. For now, just study the
source code to see what conventions are used. Any deficiences can be addressed
during code review.
