# Makefile for IA32 kernel build

PROJECT_ROOT := $(abspath ..)

-include $(PROJECT_ROOT)/Makefile.common.mk

.PHONY: all clean Core ArchIA32 PlatformPC KernelIA32PC

all: Core ArchIA32 PlatformPC KernelIA32PC

clean:
	$(call ECHO,RM $(BUILD),Quantum.Kernel)
	@rm -rf $(BUILD)

MODULES_INCLUDES := -I. -ICore
SHARED_INCLUDES  := -I$(PROJECT_INCLUDE) $(MODULES_INCLUDES) \
	-I$(PROJECT_ROOT)/Include/Quantum/Kernel

# Core

KERNEL_CORE_CPPS  := \
	$(wildcard *.cpp) \
	$(wildcard Core/*.cpp) \
	$(wildcard Core/*/*.cpp) \
	$(wildcard Core/*/*/*.cpp) \
	$(wildcard Concurrency/*.cpp) \
	$(wildcard Concurrency/*/*.cpp) \
	$(wildcard Memory/*.cpp) \
	$(wildcard Memory/*/*.cpp) \
	$(wildcard IPC/*.cpp) \
	$(wildcard IPC/*/*.cpp) \
	$(wildcard Resources/*.cpp) \
	$(wildcard Handlers/*.cpp)
KERNEL_CORE_CPP_OBJS := \
	$(patsubst \
		%.cpp, \
		$(BUILD)/%.cpp.o, \
		$(KERNEL_CORE_CPPS) \
	)
KERNEL_CORE_OBJS := $(KERNEL_CORE_CPP_OBJS)
KERNEL_CORE_INCLUDES := $(SHARED_INCLUDES)

Core: $(KERNEL_CORE_OBJS)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Kernel)
	@$(CC32) $(CFLAGS32) $(KERNEL_CORE_INCLUDES) -c $< -o $@

# Arch - IA32

KERNEL_ARCH_IA32           := Arch/IA32
KERNEL_ARCH_IA32_BUILD     := $(BUILD)/$(KERNEL_ARCH_IA32)
KERNEL_ARCH_IA32_CPPS      := \
	$(shell find $(KERNEL_ARCH_IA32) -type f -name '*.cpp')
KERNEL_ARCH_IA32_CPP_OBJS  := \
	$(patsubst \
		$(KERNEL_ARCH_IA32)/%.cpp, \
		$(KERNEL_ARCH_IA32_BUILD)/%.cpp.o, \
		$(KERNEL_ARCH_IA32_CPPS) \
	)
KERNEL_ARCH_IA32_ASMS      := \
	$(shell find $(KERNEL_ARCH_IA32) -type f -name '*.asm')
KERNEL_ARCH_IA32_ASM_OBJS  := \
	$(patsubst \
		$(KERNEL_ARCH_IA32)/%.asm, \
		$(KERNEL_ARCH_IA32_BUILD)/%.asm.o, \
		$(KERNEL_ARCH_IA32_ASMS) \
	)
KERNEL_ARCH_IA32_OBJS := \
	$(KERNEL_ARCH_IA32_CPP_OBJS) \
	$(KERNEL_ARCH_IA32_ASM_OBJS)
KERNEL_ARCH_IA32_INCLUDES := \
	$(SHARED_INCLUDES) \
	-I$(KERNEL_ARCH_IA32)

ArchIA32: $(KERNEL_ARCH_IA32_OBJS)

$(KERNEL_ARCH_IA32_BUILD)/%.cpp.o: $(KERNEL_ARCH_IA32)/%.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Kernel)
	@$(CC32) $(CFLAGS32) $(KERNEL_ARCH_IA32_INCLUDES) -c $< -o $@

$(KERNEL_ARCH_IA32_BUILD)/%.asm.o: $(KERNEL_ARCH_IA32)/%.asm
	@mkdir -p $(dir $@)
	$(call ECHO,ASM $(REL)/$<,Quantum.Kernel)
	@$(ASM) -f elf32 $< -o $@

# Platform - PC

KERNEL_PLATFORM_PC           := Platform/PC
KERNEL_PLATFORM_PC_BUILD     := $(BUILD)/$(KERNEL_PLATFORM_PC)
KERNEL_PLATFORM_PC_CPPS      := \
	$(shell find $(KERNEL_PLATFORM_PC) -type f -name '*.cpp')
KERNEL_PLATFORM_PC_CPP_OBJS  := \
	$(patsubst \
		$(KERNEL_PLATFORM_PC)/%.cpp, \
		$(KERNEL_PLATFORM_PC_BUILD)/%.cpp.o, \
		$(KERNEL_PLATFORM_PC_CPPS) \
	)
KERNEL_PLATFORM_PC_OBJS := $(KERNEL_PLATFORM_PC_CPP_OBJS)
KERNEL_PLATFORM_PC_INCLUDES := $(SHARED_INCLUDES) \
	-I$(KERNEL_PLATFORM_PC)

PlatformPC: $(KERNEL_PLATFORM_PC_CPP_OBJS)

$(KERNEL_PLATFORM_PC_BUILD)/%.cpp.o: $(KERNEL_PLATFORM_PC)/%.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Kernel)
	@$(CC32) $(CFLAGS32) $(KERNEL_PLATFORM_PC_INCLUDES) -c $< -o $@

# Kernel - IA32 PC

KERNEL_IA32_PC  := $(KERNEL)
KERNEL_IA32_PC_ELF := $(KERNEL_IA32_PC).elf
KERNEL_IA32_PC_OBJS := \
	$(KERNEL_CORE_OBJS) \
	$(KERNEL_ARCH_IA32_OBJS) \
	$(KERNEL_PLATFORM_PC_OBJS) \
	$(CORE)

KernelIA32PC: $(KERNEL_IA32_PC)

$(KERNEL_IA32_PC): $(KERNEL_IA32_PC_OBJS) $(KERNEL_LINKER) $(CORE)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Kernel)
	@$(CC32) $(CFLAGS32) -m32 -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-T $(KERNEL_LINKER) \
		$(KERNEL_IA32_PC_OBJS) -o $(KERNEL_IA32_PC_ELF)
	$(call ECHO,OBJCOPY $(REL)/$@,Quantum.Kernel)
	@$(OBJCOPY32) -O binary $(KERNEL_IA32_PC_ELF) $@

-include $(KERNEL_CORE_CPP_OBJS:.o=.d)
-include $(KERNEL_ARCH_IA32_CPP_OBJS:.o=.d)
-include $(KERNEL_PLATFORM_PC_CPP_OBJS:.o=.d)
