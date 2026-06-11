# Makefile for Quantum library

PROJECT_ROOT ?= $(abspath ..)

-include $(PROJECT_ROOT)/Makefile.common.mk

CORE_LIB := $(CORE)
CORE_QLL := $(BUILD)/Quantum.Core.qll
INCLUDE := $(PROJECT_ROOT)/Include/Quantum/Core

CFLAGS32_PIC = $(CFLAGS32) -fPIC

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS))
PIC_OBJS := $(patsubst %.cpp,$(BUILD)/%.pic.o,$(SRCS))

.PHONY: all clean Quantum qll

all: Quantum

Quantum: $(CORE_LIB)

qll: $(CORE_QLL)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Core)
	@$(CC32) $(CFLAGS32) -I$(INCLUDE) -I$(PROJECT_ROOT)/Include/ -c $< -o $@

$(BUILD)/%.pic.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC-PIC $(REL)/$<,Quantum.Core)
	@$(CC32) $(CFLAGS32_PIC) -I$(INCLUDE) -I$(PROJECT_ROOT)/Include/ -c $< -o $@

$(CORE_LIB): $(OBJS)
	@mkdir -p $(dir $@)
	$(call ECHO,AR $(REL)/$@,Quantum.Core)
	@$(AR32) rcs $@ $(OBJS)
	@$(RANLIB) $@

$(BUILD)/Quantum.Core.so: $(PIC_OBJS)
	@mkdir -p $(dir $@)
	$(call ECHO,LD-SO $(REL)/$@,Quantum.Core)
	@$(CC32) -m32 -shared -nostdlib -ffreestanding \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-T $(QLL_LINKER) \
		$(PIC_OBJS) -o $@

$(CORE_QLL): $(BUILD)/Quantum.Core.so
	@mkdir -p $(dir $@)
	$(call ECHO,QLL $(REL)/$@,Quantum.Core)
	@$(QLL_PACK) $< -o $@ --name "Quantum.Core"

-include $(OBJS:.o=.d)
-include $(PIC_OBJS:.pic.o=.pic.d)

clean:
	$(call ECHO,RM $(BUILD),Quantum.Core)
	@rm -rf $(BUILD)
