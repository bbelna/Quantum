# Makefile for Quantum.Streaming library

PROJECT_ROOT ?= $(abspath ..)

-include $(PROJECT_ROOT)/Makefile.common.mk

INCLUDE := $(PROJECT_ROOT)/Include/Quantum/Streaming

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean Quantum

all: Quantum

Quantum: $(STREAMING)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Streaming)
	@$(CC32) $(CFLAGS32) -I. -I$(INCLUDE) -I$(PROJECT_ROOT)/Include/ \
		-c $< -o $@

$(STREAMING): $(OBJS)
	@mkdir -p $(dir $@)
	$(call ECHO,AR $(REL)/$@,Quantum.Streaming)
	@$(AR32) rcs $@ $(OBJS)
	@$(RANLIB) $@

-include $(OBJS:.o=.d)

clean:
	$(call ECHO,RM $(BUILD),Quantum.Streaming)
	@rm -rf $(BUILD)
