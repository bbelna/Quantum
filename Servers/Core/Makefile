# Makefile for Quantum OS Servers Core library

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

INCLUDE := $(PROJECT_ROOT)/Include/Quantum/Servers/Core

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean Core

all: Core

Core: $(SERVERS_CORE)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Servers.Core)
	@$(CC32) $(CFLAGS32) -I. -I$(INCLUDE) -I$(PROJECT_INCLUDE) -c $< -o $@

$(SERVERS_CORE): $(OBJS)
	@mkdir -p $(dir $@)
	$(call ECHO,AR $(REL)/$@,Quantum.Servers.Core)
	@$(AR32) rcs $@ $(OBJS)
	@$(RANLIB) $@

-include $(OBJS:.o=.d)

clean:
	$(call ECHO,RM $(BUILD),Quantum.Servers.Core)
	@rm -rf $(BUILD)
