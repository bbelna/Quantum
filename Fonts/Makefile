# Makefile for Quantum Fonts library

PROJECT_ROOT ?= $(abspath ..)

-include $(PROJECT_ROOT)/Makefile.common.mk

INCLUDE := $(PROJECT_ROOT)/Include/Quantum/Fonts

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean Fonts

all: Fonts

Fonts: $(FONTS)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Fonts)
	@$(CC32) $(CFLAGS32) -I$(INCLUDE) -I$(PROJECT_ROOT)/Include/ -c $< -o $@

$(FONTS): $(OBJS)
	@mkdir -p $(dir $@)
	$(call ECHO,AR $(REL)/$@,Quantum.Fonts)
	@$(AR32) rcs $@ $(OBJS)
	@$(RANLIB) $@

-include $(OBJS:.o=.d)

clean:
	$(call ECHO,RM $(BUILD),Quantum.Fonts)
	@rm -rf $(BUILD)
