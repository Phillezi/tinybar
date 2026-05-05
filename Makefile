CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -O3 -flto -march=native -MMD -MP -Ibuild -ffunction-sections -fdata-sections -fomit-frame-pointer
LDFLAGS := -lwayland-client -flto -Wl,-O,--gc-sections

SRC_DIR  := src
BUILD_DIR := build
DEPS_DIR  := deps

TARGET := tinybar

PROTO_WLR := wlr-layer-shell-unstable-v1
PROTO_XDG := xdg-shell

PROTO_WLR_XML := $(DEPS_DIR)/$(PROTO_WLR).xml
PROTO_XDG_XML := $(DEPS_DIR)/$(PROTO_XDG).xml

PROTO_WLR_C := $(BUILD_DIR)/$(PROTO_WLR).c
PROTO_WLR_H := $(BUILD_DIR)/$(PROTO_WLR).h

PROTO_XDG_C := $(BUILD_DIR)/$(PROTO_XDG).c
PROTO_XDG_H := $(BUILD_DIR)/$(PROTO_XDG).h

SRC := $(SRC_DIR)/main.c

OBJ := \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/$(PROTO_WLR).o \
	$(BUILD_DIR)/$(PROTO_XDG).o

DEPS := $(OBJ:.o=.d)


all: deps $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/main.o: $(SRC) $(PROTO_WLR_H) $(PROTO_XDG_H)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(PROTO_WLR).o: $(PROTO_WLR_C) $(PROTO_WLR_H)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(PROTO_XDG).o: $(PROTO_XDG_C) $(PROTO_XDG_H)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(PROTO_WLR_H): $(PROTO_WLR_XML)
	@mkdir -p $(BUILD_DIR)
	wayland-scanner client-header $< $@

$(PROTO_WLR_C): $(PROTO_WLR_XML)
	@mkdir -p $(BUILD_DIR)
	wayland-scanner private-code $< $@

$(PROTO_XDG_H): $(PROTO_XDG_XML)
	@mkdir -p $(BUILD_DIR)
	wayland-scanner client-header $< $@

$(PROTO_XDG_C): $(PROTO_XDG_XML)
	@mkdir -p $(BUILD_DIR)
	wayland-scanner private-code $< $@


$(PROTO_WLR_XML):
	@mkdir -p $(DEPS_DIR)
	@if [ ! -f $@ ]; then \
		echo "Downloading wlr-layer-shell..."; \
		curl -L https://raw.githubusercontent.com/swaywm/sway/master/protocols/wlr-layer-shell-unstable-v1.xml -o $@; \
	fi

$(PROTO_XDG_XML):
	@mkdir -p $(DEPS_DIR)
	@if [ ! -f $@ ]; then \
		echo "Downloading xdg-shell..."; \
		curl -L https://gitlab.freedesktop.org/wayland/wayland-protocols/-/raw/main/stable/xdg-shell/xdg-shell.xml -o $@; \
	fi

deps: $(PROTO_WLR_XML) $(PROTO_XDG_XML)


-include $(DEPS)


clean:
	rm -rf $(BUILD_DIR) $(TARGET)

distclean:
	rm -rf $(BUILD_DIR) $(DEPS_DIR) $(TARGET)


.PHONY: all clean distclean deps
