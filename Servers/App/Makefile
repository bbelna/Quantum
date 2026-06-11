# Makefile for IA32 AppServer

PROJECT_ROOT := $(abspath ../..)

-include $(PROJECT_ROOT)/Makefile.common.mk

APP_SERVER := $(BUILD)/AppServer.qx
SRCS := $(wildcard *.cpp) $(wildcard Core/*.cpp) $(wildcard Controllers/*.cpp) $(wildcard Windows/*.cpp) $(wildcard Cursor/*.cpp) $(wildcard Overlays/*.cpp) $(wildcard Dock/*.cpp)
OBJS := $(patsubst %.cpp,$(BUILD)/%.cpp.o,$(SRCS))

.PHONY: all clean AppServer

all: AppServer

-include $(OBJS:.o=.d)

AppServer: $(APP_SERVER)

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(call ECHO,CC $(REL)/$<,Quantum.Servers.App)
	@$(CC32) $(CFLAGS32) -I. -I$(PROJECT_INCLUDE) -c $< -o $@

$(APP_SERVER): $(OBJS) $(SERVERS_CORE) $(RUNTIME) $(CORE) $(CLIENTS) $(THREADING) $(FONTS) $(CURSORS) $(COMPONENTS) $(MENUS) $(UI)
	@mkdir -p $(dir $@)
	$(call ECHO,LD $(REL)/$@,Quantum.Servers.App)
	@$(CC32) $(CFLAGS32) -m32 -ffreestanding -nostdlib -static -Wl,--no-pie \
		-Wl,--no-warn-rwx-segments -Wl,--build-id=none \
		-I$(PROJECT_INCLUDE) \
		-T $(SERVER_LINKER) \
		$(OBJS) $(SERVERS_CORE) $(RUNTIME) $(CORE) $(CLIENTS) $(THREADING) $(FONTS) $(CURSORS) $(COMPONENTS) $(MENUS) $(UI) -o $@

clean:
	$(call ECHO,RM $(BUILD),Quantum.Servers.App)
	@rm -rf $(BUILD)
