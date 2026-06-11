# Makefile for UIShowcase

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

UI_SHOWCASE := $(BUILD)/UIShowcase.qapp
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean UIShowcase

all: UIShowcase

-include $(OBJS:.o=.d)

UIShowcase: $(UI_SHOWCASE)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Demos.UIShowcase)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_INCLUDE) -c $< -o $@

$(UI_SHOWCASE): $(OBJS) $(RUNTIME) $(CORE) $(THREADING) $(FONTS) $(APP_LIB) $(COMPONENTS) $(MENUS) $(UI)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Demos.UIShowcase)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(USER_LINKER) \
		$(OBJS) $(APP_LIB) $(COMPONENTS) $(MENUS) $(UI) $(CORE) $(THREADING) $(FONTS) $(RUNTIME) -o $@.elf
	$(call ECHO,QXF $(REL)/$@,Quantum.Apps.Demos.UIShowcase)
	@$(QXF_PACK) $@.elf -o $@ --type qapp

clean:
	$(call ECHO,RM $(BUILD),Quantum.Demos.UIShowcase)
	@rm -rf $(BUILD)
