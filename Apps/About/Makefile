# Makefile for IA32 About

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

ABOUT_BIN := $(BUILD)/About.qapp
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean About

all: About

-include $(OBJS:.o=.d)

About: $(ABOUT_BIN)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Apps.About)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_INCLUDE) -c $< -o $@

$(ABOUT_BIN): $(OBJS) $(CORE) $(THREADING) $(FONTS) $(APP_LIB) $(COMPONENTS) $(MENUS) $(UI) $(RUNTIME)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Apps.About)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(APP_LIB) $(COMPONENTS) $(MENUS) $(UI) $(CORE) $(THREADING) $(FONTS) $(RUNTIME) -o $@.elf
	$(call ECHO,QXF $(REL)/$@,Quantum.Apps.About)
	@$(QXF_PACK) $@.elf -o $@ --type qapp

clean:
	$(call ECHO,RM $(BUILD),Quantum.Apps.About)
	@rm -rf $(BUILD)
