# Phosphene

A C++ library for building emulator frontends on macOS. Provides GPU-accelerated graphics (2D and 3D), audio playback with frame-based synchronization, and gamepad input handling.

## Platform support

| Platform | Status |
|----------|--------|
| macOS (Metal) | Supported |
| Linux (Vulkan) | Planned — SPIR-V shaders not yet embedded |
| Windows (DirectX/Vulkan) | Planned — SPIR-V shaders not yet embedded |

The `Renderer` class currently embeds MSL shaders only. `Display`, `Audio`, and `Input` use SDL3 abstractions and are portable, but a full cross-platform build requires pre-compiled SPIR-V bytecode to be added for Vulkan. Contributions welcome.

## Features

- **GPU-Accelerated Rendering**
  - **Display**: Simple 2D framebuffer renderer for pixel-based systems (NES, SNES, Genesis, etc.)
  - **Renderer**: Advanced 3D renderer for transformed geometry and textured draws (PS1, N64 style) — macOS only for now (see platform support above)
  - Built on SDL3 GPU API

- **High-Quality Audio**
  - Frame-based audio synchronization for emulation accuracy
  - Real-time audio resampling (libsoxr)
  - Dynamic Rate Correction (DRC) to handle clock drift

- **Input Handling**
  - Gamepad/joystick support via SDL3
  - Keyboard fallback (arrow keys + ZXRS)

## Requirements

- **C++17 compiler** (GCC, Clang, MSVC)
- **SDL3** — graphics, audio, input
- **libsoxr** — audio resampling
- **CMake 3.16+** — recommended build system for integration
- **pkg-config** — used by the Makefile build

## Installation

### macOS

```bash
# Install dependencies via Homebrew
brew install sdl3 libsoxr pkg-config xcode-select

# (If needed) Install Xcode Command Line Tools
xcode-select --install

# Build library and example
make
```

### Linux (Debian 13+)

```bash
sudo apt update
sudo apt install build-essential pkg-config libsdl3-dev libsoxr-dev

make
```

### Windows

```bash
# Via MSYS2/MinGW-w64
pacman -S mingw-w64-x86_64-sdl3 mingw-w64-x86_64-libsoxr

make
```

> **Note:** On Linux and Windows, `Display`, `Audio`, `Input`, and `Resampler` all work. `Renderer` will throw at runtime on non-Metal backends until SPIR-V shader support is added (see [platform support](#platform-support)).

## Building

### Quick Start

```bash
# Build everything (static lib, dynamic lib, example)
make

# Build just the static library
make static

# Build the example binary
make example

# Run the example
./build/example

# Clean build artifacts
make clean
```

### Installation

Install the library and headers system-wide (defaults to `/usr/local`):

```bash
make install                        # installs to /usr/local
make install PREFIX=~/.local        # installs to a custom prefix
```

This copies libraries to `$(PREFIX)/lib/` and headers to `$(PREFIX)/include/phosphene/`.

### Configuration

Override variables on the command line:

```bash
# Use a different compiler
make CXX=clang++

# Build with debug symbols
make CXXFLAGS="-std=c++17 -Wall -Wextra -g"

# See all detected settings
make info
```

## Using Phosphene in Your Project

Phosphene uses CMake and exposes the `Phosphene::phosphene` target. There are two integration paths depending on whether you want to vendor the source or install it system-wide.

### Option A: Git Submodule (recommended)

No install step required — CMake builds Phosphene as part of your project.

```bash
git submodule add https://github.com/ooPo/Phosphene vendor/phosphene
```

In your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyEmulator)

add_subdirectory(vendor/phosphene)

add_executable(my_emu main.cpp)
target_link_libraries(my_emu PRIVATE Phosphene::phosphene)
```

SDL3 and libsoxr must be installed on the build machine (see [Installation](#installation)).

### Option B: System Install + find_package

Build and install Phosphene to a prefix, then use `find_package` from any project.

```bash
cmake -S path/to/phosphene -B phosphene_build
cmake --install phosphene_build --prefix /usr/local   # or any prefix
```

In your `CMakeLists.txt`:

```cmake
find_package(Phosphene REQUIRED)

add_executable(my_emu main.cpp)
target_link_libraries(my_emu PRIVATE Phosphene::phosphene)
```

If you installed to a custom prefix, point CMake to it:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/prefix
```

### Compiler and include setup

The `Phosphene::phosphene` target propagates everything automatically — include paths, link libraries, and the C++17 requirement. No manual `include_directories` or `target_compile_options` calls are needed.

Your source files include headers as:

```cpp
#include <phosphene/window.h>
#include <phosphene/display.h>
#include <phosphene/audio.h>
#include <phosphene/resampler.h>
#include <phosphene/input.h>
```

## Project Structure

```
.
├── include/
│   └── phosphene/          # Public headers
│       ├── window.h
│       ├── display.h
│       ├── renderer.h
│       ├── audio.h
│       ├── resampler.h
│       ├── input.h
│       ├── math3d.h
│       └── span.h
├── src/                       # Implementation
│   ├── window.cpp
│   ├── display.cpp
│   ├── renderer.cpp
│   ├── audio.cpp
│   ├── resampler.cpp
│   ├── input.cpp
│   └── math3d.cpp
├── examples/
│   ├── basic/                # Colour-cycling 2D display + audio + DRC
│   ├── cube/                 # Untextured spinning cube
│   ├── textured_cube/        # Textured spinning cube
│   └── hud/                  # 3D scene with alpha-blended HUD overlay
├── build/                    # Build output (generated)
├── Makefile
└── README.md                 # This file
```

## Usage

### Basic Example

```cpp
#include <phosphene/window.h>
#include <phosphene/display.h>
#include <phosphene/audio.h>
#include <phosphene/input.h>

int main() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);

    // Initialize context
    Window ctx;
    ctx.init("My Emulator", 512, 480);

    // Initialize components
    Display video;
    video.init(ctx, 256, 240);  // NES framebuffer size

    Audio audio;
    audio.init(44100, 1, 60.0);  // 44.1 kHz mono @ 60 FPS

    Input input;
    input.init();

    // Main loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            input.handle_event(event);
        }

        // Get input
        InputState state = input.read();
        // ... use state.up, state.down, state.a, state.b, etc.

        // Frame pacing
        audio.wait_for_frame();

        // Generate and present framebuffer
        uint32_t framebuffer[256 * 240];
        // ... fill framebuffer with emulated output
        Framebuffer fb = { framebuffer, 256, 240 };
        video.present(fb);

        // Generate and push audio samples
        float samples[1024];
        // ... fill with emulated audio
        audio.push(samples, 1024);
    }

    input.shutdown();
    audio.shutdown();
    video.shutdown();
    ctx.shutdown();
    SDL_Quit();
    return 0;
}
```

## Key Classes

### `Window`
Manages SDL3 GPU device and window lifecycle. Initialize once at startup.

**Methods:**
- `init(title, width, height)` — Create window and GPU device
- `gpu()` — Get GPU device handle
- `window()` — Get window handle
- `shutdown()` — Clean up resources

### `Display`
Simple 2D renderer for pixel-based emulators.

**Methods:**
- `init(ctx, fb_width, fb_height)` — Initialize with framebuffer dimensions
- `present(framebuffer)` — Upload and display framebuffer each frame
- `shutdown()` — Clean up GPU texture

**Usage:**
```cpp
Framebuffer fb = { pixel_data, width, height };
video.present(fb);
```

### `Renderer`
Advanced 3D renderer for transformed geometry.

**Methods:**
- `init(ctx, render_width, render_height)` — Set internal render resolution
- `begin_frame()` — Start a new render pass
- `submit_draw(cmd)` — Queue a draw call
- `end_frame()` — Flush and present
- `create_texture(width, height)` — Allocate GPU texture
- `destroy_texture(tex)` — Release texture

### `Audio`
Frame-synchronized audio playback with resampling support.

**Methods:**
- `init(sample_rate, channels, emulated_fps)` — Initialize device
- `push(samples, count)` — Enqueue audio samples
- `wait_for_frame()` — Block until audio buffer ready for next frame
- `compute_drc_rate(resampler)` — Compute DRC rate adjustment
- `shutdown()` — Clean up device

### `Resampler`
High-quality sample rate conversion (libsoxr wrapper).

**Methods:**
- `init(in_rate, out_rate, channels)` — Initialize resampler
- `process(in, in_count, out, out_capacity)` — Resample audio
- `set_out_rate(new_rate)` — Update output rate (for DRC)
- `shutdown()` — Clean up

### `Input`
Gamepad and keyboard input handling.

**Methods:**
- `init()` — Initialize input system
- `handle_event(event)` — Process SDL events
- `read()` — Get current button state
- `shutdown()` — Clean up

**Input Mappings:**
- **Gamepad**: D-pad, A/B buttons, Select, Start
- **Keyboard**: Arrow keys, Z (A), X (B), Right Shift (Select), Enter (Start)

## Architecture Notes

- **Non-owning pointers**: `Display` and `Renderer` hold non-owning references to `Window`
- **Forward declarations**: Headers minimize coupling (e.g., `Resampler` forward-declared in `Audio`)
- **Exception safety**: Initialize/shutdown pattern with cleanup on error
- **GPU resource management**: SDL3 GPU API handles memory; you call `destroy_texture()` for manual releases

## Performance Considerations

- **Frame pacing**: `audio.wait_for_frame()` blocks until the audio buffer reaches a target level, providing natural frame pacing
- **DRC**: `audio.compute_drc_rate()` adjusts the resampler output rate to keep audio synchronized with video
- **Nearest-neighbour filtering**: `Display` uses nearest-neighbour scaling for authentic retro appearance
- **Depth testing**: `Renderer` includes Z-buffering for correct 3D rendering

## Examples

Four examples are included, each targeting a different feature set:

| Example | What it shows |
|---------|--------------|
| `basic` | 2D colour-cycling display, sine audio, DRC, window scaling |
| `cube` | Untextured spinning cube with the 3D renderer |
| `textured_cube` | Textured spinning cube with checkerboard texture upload |
| `hud` | 3D scene with an alpha-blended HUD overlay composited on top |

Build and run with the Makefile:

```bash
make example          # builds and links examples/basic
./build/example

make                  # builds all targets including cube, textured_cube, hud
./build/cube
./build/textured_cube
./build/hud
```

Or via CMake with `-DPHOSPHENE_BUILD_EXAMPLES=ON`.

## Contributing

When adding new features:
1. Update relevant headers with Doxygen-style documentation
2. Maintain consistent code style (see existing files)
3. Test on supported platforms
4. Update README if API changes

## License

BSD 2-Clause. See [LICENSE.txt](LICENSE.txt).

## Troubleshooting

**Compilation errors for SDL3?**
- Verify SDL3 is installed: `pkg-config --cflags sdl3`
- Try `make info` to see detected paths
- On macOS: `brew install sdl3`
- On Linux: `sudo apt install libsdl3-dev`

**Audio not playing?**
- Check system volume and audio device
- Verify `soxr_create()` succeeds: run the example and look for errors
- Ensure output sample rate matches hardware

**Example window not appearing?**
- On Linux/Wayland, SDL3 may require additional environment variables
- Try `SDL_VIDEODRIVER=x11 ./build/example` on Linux

## References

- [SDL3 Documentation](https://wiki.libsdl.org/SDL3)
- [libsoxr](https://sourceforge.net/projects/soxr/)
