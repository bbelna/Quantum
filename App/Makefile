# Makefile for Quantum App library

PROJECT_ROOT ?= $(abspath ..)

-include $(PROJECT_ROOT)/Makefile.common.mk

INCLUDE := $(PROJECT_ROOT)/Include/Quantum/App

SRCS := $(wildcard *.cpp) $(wildcard OS/QuantumOS/*.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean Quantum.App

all: Quantum.App

Quantum.App: $(APP_LIB)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.App)
	@$(CC32) $(CFLAGS32) -I. -I$(INCLUDE) -I$(PROJECT_INCLUDE) \
		-c $< -o $@

$(APP_LIB): $(OBJS)
	@mkdir -p $(dir $@)
	$(call ECHO,AR $(REL)/$@,Quantum.App)
	@$(AR32) rcs $@ $(OBJS)
	@$(RANLIB) $@

-include $(OBJS:.o=.d)

clean:
	$(call ECHO,RM $(BUILD),Quantum.App)
	@rm -rf $(BUILD)
