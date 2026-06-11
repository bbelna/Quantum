# Makefile for Quantum
#
# Top-level build orchestrator. Builds all packages in dependency order
# for the configured PLATFORM and ARCH. Does NOT create disk images;
# use the appropriate Toolchain/Platform/ script for that.

.DEFAULT_GOAL := default

PROJECT_ROOT := .

-include Makefile.config.mk
-include Makefile.common.mk

# Packages in dependency order (libraries, then apps/utils, then OS)
PACKAGES := \
	Core \
	Runtime \
	Clients \
	Servers/Core \
	Threading \
	Fonts \
	Cursors \
	UI \
	Components \
	Menus \
	App \
	Streaming \
	Streaming \
	Utilities/echo \
	Utilities/cwd \
	Utilities/ls \
	Utilities/vols \
	Utilities/cat \
	Utilities/ps \
	Utilities/mem \
	Utilities/kmem \
	Utilities/schstat \
	Utilities/devstat \
	Utilities/ver \
	Utilities/qsh \
	Apps/UIShowcase \
	Apps/About \
	Apps/Terminal \
	Bootloader \
	Drivers/Input/PS2Keyboard \
	Drivers/Input/PS2Mouse \
	Drivers/Storage/FDC \
	Drivers/Storage/ATA \
	FileSystems/FAT12 \
	Servers/Startup \
	Servers/Run \
	Servers/Device \
	Servers/FileSystem \
	Servers/Storage \
	Servers/Graphics \
	Servers/Input \
	Servers/Stream \
	Servers/App \
	Servers/Context \
	Kernel

.PHONY: default all clean

default: all

all:
	@for pkg in $(PACKAGES); do \
		printf "%s%s\n" "BUILD Quantum: " "$$pkg"; \
		$(MAKE) -s -C "$$pkg" all || exit 1; \
	done

clean:
	@for pkg in $(PACKAGES); do \
		$(MAKE) -s -C "$$pkg" clean 2>/dev/null || true; \
	done
	@rm -rf $(BUILD_ROOT)
