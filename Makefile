# ==============================================================
#  Phosphene — Makefile
#  
#  A cross-platform C++ library for emulator frontends, providing:
#    - SDL3-based GPU rendering (2D and 3D)
#    - Audio playback with frame-based pacing
#    - Gamepad input handling
#    - High-quality audio resampling (libsoxr)
#
#  Supports: macOS (Metal), Linux (Vulkan/OpenGL), Windows (DirectX/Vulkan)
#
#  Targets:
#    make             — build both static and dynamic libraries + example
#    make static      — build static library only
#    make dynamic     — build dynamic library only
#    make example     — build example binary
#    make run         — build and run the example
#    make install     — install libraries and headers locally
#    make clean       — remove all build artifacts
#    make rebuild     — clean then build all
#    make info        — print build configuration
# ==============================================================

# Configuration
CXX      ?= g++
AR       ?= ar
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -fPIC

# Directories
SRC_DIR       := src
INCLUDE_DIR   := include
BUILD_DIR     := build
EXAMPLE_DIR   := examples/basic

# Output paths
STATIC_LIB    := $(BUILD_DIR)/libphosphene.a
DYNAMIC_LIB   := 
EXAMPLE_BIN   := $(BUILD_DIR)/example

# Install paths — override with: make install PREFIX=/your/path
PREFIX          ?= /usr/local
INSTALL_LIB     := $(PREFIX)/lib
INSTALL_INCLUDE := $(PREFIX)/include/phosphene

# ==============================================================
#  Platform detection
# ==============================================================

UNAME := $(shell uname -s)

ifeq ($(UNAME), Darwin)
    PLATFORM := macos
else ifeq ($(OS), Windows_NT)
    PLATFORM := windows
else
    PLATFORM := linux
endif

# Platform-specific settings
ifeq ($(PLATFORM), macos)
    DYNAMIC_LIB  := $(BUILD_DIR)/libphosphene.dylib
    FRAMEWORKS   := -framework Metal \
                    -framework MetalKit \
                    -framework QuartzCore \
                    -framework Cocoa \
                    -framework IOKit \
                    -framework CoreAudio \
                    -framework AudioToolbox \
                    -framework GameController
    LDFLAGS      := $(FRAMEWORKS)
    DYNAMIC_FLAGS := -dynamiclib
else ifeq ($(PLATFORM), windows)
    DYNAMIC_LIB   := $(BUILD_DIR)/phosphene.dll
    LDFLAGS       := -mwindows
    DYNAMIC_FLAGS := -shared
else
    DYNAMIC_LIB   := $(BUILD_DIR)/libphosphene.so
    LDFLAGS       :=
    DYNAMIC_FLAGS := -shared
    CXXFLAGS      += -fPIC
endif

# ==============================================================
#  Dependency detection (SDL3 + libsoxr)
# ==============================================================

SDL3_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null || echo "-I/usr/local/include/SDL3")
SDL3_LIBS   := $(shell pkg-config --libs   sdl3 2>/dev/null || echo "-lSDL3")
SOXR_CFLAGS := $(shell pkg-config --cflags soxr 2>/dev/null || echo "-I/usr/local/include")
SOXR_LIBS   := $(shell pkg-config --libs   soxr 2>/dev/null || echo "-lsoxr")

PKG_CFLAGS := $(SDL3_CFLAGS) $(SOXR_CFLAGS)
PKG_LIBS   := $(SDL3_LIBS) $(SOXR_LIBS) $(LDFLAGS)

# ==============================================================
#  Source files
# ==============================================================

# Library sources
LIB_SRCS := $(SRC_DIR)/window.cpp \
            $(SRC_DIR)/display.cpp \
            $(SRC_DIR)/renderer.cpp \
            $(SRC_DIR)/audio.cpp \
            $(SRC_DIR)/resampler.cpp \
            $(SRC_DIR)/input.cpp \
            $(SRC_DIR)/math3d.cpp

# Example sources
EXAMPLE_SRC := $(EXAMPLE_DIR)/main.cpp
CUBE_DIR    := examples/cube
CUBE_SRC    := $(CUBE_DIR)/main.cpp
CUBE_BIN    := $(BUILD_DIR)/cube

TCUBE_DIR   := examples/textured_cube
TCUBE_SRC   := $(TCUBE_DIR)/main.cpp
TCUBE_BIN   := $(BUILD_DIR)/textured_cube

HUD_DIR     := examples/hud
HUD_SRC     := $(HUD_DIR)/main.cpp
HUD_BIN     := $(BUILD_DIR)/hud

# Object files (output to build dir)
LIB_OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(LIB_SRCS))
EXAMPLE_OBJ  := $(BUILD_DIR)/example_main.o
CUBE_OBJ     := $(BUILD_DIR)/cube_main.o
TCUBE_OBJ    := $(BUILD_DIR)/textured_cube_main.o
HUD_OBJ      := $(BUILD_DIR)/hud_main.o
ALL_OBJS     := $(LIB_OBJS) $(EXAMPLE_OBJ) $(CUBE_OBJ) $(TCUBE_OBJ) $(HUD_OBJ)
DEPS         := $(ALL_OBJS:.o=.d)

# Public headers (relative to include/phosphene/)
HEADERS := window.h \
           display.h \
           renderer.h \
           audio.h \
           resampler.h \
           input.h \
           span.h \
           math3d.h

HEADER_PATHS := $(addprefix $(INCLUDE_DIR)/phosphene/,$(HEADERS))

# ==============================================================
#  Compile flags
# ==============================================================

INCLUDE_FLAGS := -I$(INCLUDE_DIR) $(PKG_CFLAGS)
CXXFLAGS      += $(INCLUDE_FLAGS)

# ==============================================================
#  Phony targets
# ==============================================================

.PHONY: all static dynamic example run install clean rebuild info directories cube textured_cube hud run-cube run-textured-cube run-hud

# ==============================================================
#  Top-level targets
# ==============================================================

all: directories static dynamic example cube textured_cube hud
	@echo "✓ Build complete: $(STATIC_LIB) $(DYNAMIC_LIB) $(EXAMPLE_BIN) $(CUBE_BIN) $(TCUBE_BIN) $(HUD_BIN)"

static: directories $(STATIC_LIB)
	@echo "✓ Static library: $(STATIC_LIB)"

dynamic: directories $(DYNAMIC_LIB)
	@echo "✓ Dynamic library: $(DYNAMIC_LIB)"

example: directories $(EXAMPLE_BIN)
	@echo "✓ Example binary: $(EXAMPLE_BIN)"

cube: directories $(CUBE_BIN)
	@echo "✓ Cube example: $(CUBE_BIN)"

textured_cube: directories $(TCUBE_BIN)
	@echo "✓ Textured cube example: $(TCUBE_BIN)"

run: example
	$(EXAMPLE_BIN)

run-cube: cube
	$(CUBE_BIN)

run-textured-cube: textured_cube
	$(TCUBE_BIN)

hud: directories $(HUD_BIN)
	@echo "✓ HUD overlay example: $(HUD_BIN)"

run-hud: hud
	$(HUD_BIN)

# ==============================================================
#  Build rules
# ==============================================================

# Ensure build directory exists
directories:
	@mkdir -p $(BUILD_DIR)

# Static library (archive)
$(STATIC_LIB): $(LIB_OBJS)
	@echo "Archiving static library..."
	$(AR) rcs $@ $^

# Dynamic library (shared object)
$(DYNAMIC_LIB): $(LIB_OBJS)
	@echo "Linking dynamic library..."
	$(CXX) $(DYNAMIC_FLAGS) -o $@ $^ $(PKG_LIBS)

# Example binary (linked against static library)
$(EXAMPLE_BIN): $(EXAMPLE_OBJ) $(STATIC_LIB)
	@echo "Linking example binary..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(PKG_LIBS)

# Compile library sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

# Compile example source
$(BUILD_DIR)/example_main.o: $(EXAMPLE_SRC)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

# Cube example
$(CUBE_BIN): $(CUBE_OBJ) $(STATIC_LIB)
	@echo "Linking cube binary..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(PKG_LIBS)

$(BUILD_DIR)/cube_main.o: $(CUBE_SRC)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

# Textured cube example
$(TCUBE_BIN): $(TCUBE_OBJ) $(STATIC_LIB)
	@echo "Linking textured cube binary..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(PKG_LIBS)

$(BUILD_DIR)/textured_cube_main.o: $(TCUBE_SRC)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

# HUD overlay example
$(HUD_BIN): $(HUD_OBJ) $(STATIC_LIB)
	@echo "Linking HUD overlay binary..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(PKG_LIBS)

$(BUILD_DIR)/hud_main.o: $(HUD_SRC)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

# Include dependency files
-include $(DEPS)

# ==============================================================
#  Installation
# ==============================================================

install: static dynamic
	@echo "Installing to $(INSTALL_LIB) and $(INSTALL_INCLUDE)..."
	@mkdir -p $(INSTALL_LIB)
	@mkdir -p $(INSTALL_INCLUDE)
	@cp $(STATIC_LIB)  $(INSTALL_LIB)/
	@cp $(DYNAMIC_LIB) $(INSTALL_LIB)/
	@cp $(HEADER_PATHS) $(INSTALL_INCLUDE)/
	@echo "✓ Installation complete"

# ==============================================================
#  Cleaning
# ==============================================================

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "✓ Clean complete"

rebuild: clean all

# ==============================================================
#  Debugging
# ==============================================================

info:
	@echo "=== Build Configuration ==="
	@echo "Platform       : $(PLATFORM)"
	@echo "CXX            : $(CXX)"
	@echo "CXXFLAGS       : $(CXXFLAGS)"
	@echo ""
	@echo "=== Paths ==="
	@echo "SRC_DIR        : $(SRC_DIR)"
	@echo "INCLUDE_DIR    : $(INCLUDE_DIR)"
	@echo "BUILD_DIR      : $(BUILD_DIR)"
	@echo ""
	@echo "=== Dependencies ==="
	@echo "SDL3_CFLAGS    : $(SDL3_CFLAGS)"
	@echo "SDL3_LIBS      : $(SDL3_LIBS)"
	@echo "SOXR_CFLAGS    : $(SOXR_CFLAGS)"
	@echo "SOXR_LIBS      : $(SOXR_LIBS)"
	@echo ""
	@echo "=== Output Files ==="
	@echo "STATIC_LIB     : $(STATIC_LIB)"
	@echo "DYNAMIC_LIB    : $(DYNAMIC_LIB)"
	@echo "EXAMPLE_BIN    : $(EXAMPLE_BIN)"
	@echo ""
	@echo "=== Library Sources ==="
	@echo "$(LIB_SRCS)"
