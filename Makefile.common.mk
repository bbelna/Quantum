-include $(PROJECT_ROOT)/Makefile.config.mk

PROJECT_INCLUDE = $(PROJECT_ROOT)/Include
BUILD_ROOT = $(PROJECT_ROOT)/Build

SRC = .

# Per-project build output dir, rooted at $(BUILD_ROOT)/<project-relative-path>.
HERE     = $(CURDIR)
REL_PATH = $(shell realpath --relative-to='$(PROJECT_ROOT)' '$(HERE)')
REL      = ./$(REL_PATH)
BUILD    = $(BUILD_ROOT)/$(REL_PATH)

ASM = nasm
AR32 = ar
RANLIB = ranlib
CC32 = gcc
CFLAGS32 = -std=c++20 -fno-pic -fno-pie -m32 -march=$(TARGET_ARCH) \
	-ffreestanding -O2 -Wall -fno-exceptions -fno-rtti -nostdinc -nostdinc++ \
	-fno-stack-protector -fno-builtin -mno-sse -mno-sse2 -mno-mmx -MMD -MP \
	-DOS_QUANTUMOS -DARCH_$(ARCH) -DARCH="\"$(ARCH)\"" -DPLATFORM_$(PLATFORM) \
	-DPLATFORM="\"$(PLATFORM)\"" -DTARGET_ARCH_$(TARGET_ARCH) \
	-DTARGET_ARCH="\"$(TARGET_ARCH)\"" -ffunction-sections -fdata-sections \
	$(EXTRA_CFLAGS32) -Wno-changes-meaning
LD32 = ld
LDFLAGS32 = --no-pie -m elf_i386 --gc-sections
OBJCOPY32 = objcopy
QXF_PACK = python3 $(PROJECT_ROOT)/Toolchain/QXF/qxf-pack.py
QLL_PACK = python3 $(PROJECT_ROOT)/Toolchain/QLL/qll-pack.py

# Centralized linker scripts (Toolchain/Arch/<arch>/Linkers/).
LINKER_DIR        = $(PROJECT_ROOT)/Toolchain/Arch/IA32/Linkers
KERNEL_LINKER     = $(LINKER_DIR)/IA32-Kernel.ld
BOOTLOADER_LINKER = $(LINKER_DIR)/IA32-Bootloader.ld
STARTUP_LINKER    = $(LINKER_DIR)/IA32-Startup.ld
USER_LINKER       = $(LINKER_DIR)/IA32-User.ld
SERVER_LINKER     = $(LINKER_DIR)/IA32-Server.ld
QLL_LINKER        = $(LINKER_DIR)/IA32-QLL.ld

MKFS = mkfs.vfat
MCOPY = mcopy
MMD = mmd

ECHO = @printf "%s%s\n" "BUILD $(2): " "$(1)"
ECHO_VERBOSE = printf "%s%s\n" "BUILD $(2): " "$(1)"

CORE = $(BUILD_ROOT)/Core/Quantum.Core.a
THREADING = $(BUILD_ROOT)/Threading/Quantum.Threading.a
RUNTIME = $(BUILD_ROOT)/Runtime/Quantum.Runtime.a
APP_LIB = $(BUILD_ROOT)/App/Quantum.App.a
FONTS = $(BUILD_ROOT)/Fonts/Quantum.Fonts.a
CURSORS = $(BUILD_ROOT)/Cursors/Quantum.Cursors.a
UI = $(BUILD_ROOT)/UI/Quantum.UI.a
COMPONENTS = $(BUILD_ROOT)/Components/Quantum.Components.a
MENUS = $(BUILD_ROOT)/Menus/Quantum.Menus.a
CLIENTS = $(BUILD_ROOT)/Clients/Quantum.Clients.a
SERVERS_CORE = $(BUILD_ROOT)/Servers/Core/Quantum.Servers.Core.a
STREAMING = $(BUILD_ROOT)/Streaming/Quantum.Streaming.a
OS_STREAMING = $(BUILD_ROOT)/Streaming/Quantum.Streaming.a
ECHO_CMD = $(BUILD_ROOT)/Utilities/echo/echo.qx
OS_SHELL = $(BUILD_ROOT)/Utilities/qsh/qsh.qx
STARTUP = $(BUILD_ROOT)/Servers/Startup/StartupServer.qbn
RUN = $(BUILD_ROOT)/Servers/Run/RunServer.qbn
KERNEL = $(BUILD_ROOT)/Kernel/Kernel.qbn
BOOTLOADER_IA32_PC_FLOPPY = $(BUILD_ROOT)/Bootloader/IA32/PC/Floppy
BOOTLOADER_IA32_PC_FLOPPY_STAGE1 = $(BOOTLOADER_IA32_PC_FLOPPY)/Stage1.bin
BOOTLOADER_IA32_PC_FLOPPY_STAGE2 = $(BOOTLOADER_IA32_PC_FLOPPY)/Stage2.bin
BOOTLOADER_BOOT = $(BOOTLOADER_IA32_PC_FLOPPY)/Boot.qbn
BOOTLOADER_IA32_PC_HDD = $(BUILD_ROOT)/Bootloader/IA32/PC/HDD
BOOTLOADER_IA32_PC_HDD_STAGE1 = $(BOOTLOADER_IA32_PC_HDD)/Stage1.bin
BOOTLOADER_IA32_PC_HDD_STAGE2 = $(BOOTLOADER_IA32_PC_HDD)/Stage2.bin
BOOTLOADER_HDD_BOOT = $(BOOTLOADER_IA32_PC_HDD)/Boot.qbn
PS2KEYBOARD  = $(BUILD_ROOT)/Drivers/Input/PS2Keyboard/PS2Keyboard.qx
PS2MOUSE     = $(BUILD_ROOT)/Drivers/Input/PS2Mouse/PS2Mouse.qx
GRAPHICS     = $(BUILD_ROOT)/Servers/Graphics/GraphicsServer.qx
INPUT        = $(BUILD_ROOT)/Servers/Input/InputServer.qx
APP_SERVER   = $(BUILD_ROOT)/Servers/App/AppServer.qx
CONTEXT_SERVER = $(BUILD_ROOT)/Servers/Context/Content.qx
TERMINAL_APP    = $(BUILD_ROOT)/Apps/Terminal/Terminal.qapp
ABOUT_APP    = $(BUILD_ROOT)/Apps/About.qapp
UI_SHOWCASE = $(BUILD_ROOT)/Apps/UIShowcase/UIShowcase.qapp
CREDITS = $(PROJECT_ROOT)/Assets/Credits.txt
LICENSE = $(PROJECT_ROOT)/Assets/License.md

STARTUP_MANIFEST = $(PROJECT_ROOT)/Toolchain/StartupManifest.json
STARTUP_BUNDLE = $(BUILD)/Startup.qbd
STARTUP_BUNDLER = python3 $(PROJECT_ROOT)/Toolchain/CreateStartup.py
HAS_STARTUP_MANIFEST  = $(wildcard $(STARTUP_MANIFEST))

HELLO          = $(PROJECT_ROOT)/Assets/Hello.msg
DEFAULT_FONT   = $(PROJECT_ROOT)/Assets/Fonts/noto-sans-regular-14.qbf
TITLE_FONT     = $(PROJECT_ROOT)/Assets/Fonts/noto-sans-bold-12.qbf
SYSTEM_CURSORS = $(PROJECT_ROOT)/Assets/Cursors/system-cursors.qcur
