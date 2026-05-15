# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
make                  # build static lib, dynamic lib, and all examples
make static           # build static library only (build/libphosphene.a)
make dynamic          # build dynamic library only (build/libphosphene.dylib on macOS)
make example          # build basic example (build/example)
make cube             # build spinning cube example (build/cube)
make textured_cube    # build textured cube example (build/textured_cube)
make hud              # build HUD overlay example (build/hud)
make run              # build and run basic example
make run-cube         # build and run cube example
make run-textured-cube # build and run textured cube example
make run-hud          # build and run HUD overlay example
make install          # install libs and headers to /usr/local (PREFIX= to override)
make clean            # remove build/ directory
make rebuild          # clean then build all
make info             # print detected compiler flags, SDL3/soxr paths, output paths
```

Dependencies: SDL3, libsoxr, pkg-config. On macOS: `brew install sdl3 libsoxr pkg-config`.

The standard is **C++17** (`-std=c++17`). There are no tests and no linter configured.

## Architecture

`Window` owns the SDL3 GPU device and window. All other classes hold a **non-owning pointer** to it and require it to remain valid for their lifetime. Shutdown order must be the reverse of init order.

**Frame loop contract** (see `examples/basic/main.cpp`):
1. `audio.wait_for_frame()` — blocks until the audio queue drains to ~2 frames, providing frame pacing
2. `audio.compute_drc_rate(resampler)` + `resampler.set_out_rate(rate)` — adjust resampler output rate each frame to correct clock drift (DRC)
3. Generate video and audio data
4. `audio.push()` + `video.present()` — submit to SDL3

**Display** (`src/display.cpp`): `init()` creates a persistent GPU texture and a persistent transfer buffer. Each `present()` maps the transfer buffer, copies the framebuffer in, runs a copy pass into the GPU texture, then blits to the swapchain with nearest-neighbour filtering.

**Renderer** (`src/renderer.cpp`): fully implemented. `init()` creates four pipelines (textured, untextured, textured_alpha, untextured_alpha) via `create_pipeline()`. `submit_draw()` queues draws into `m_pending_draws`; `end_frame()` uploads all geometry in one copy pass and issues the render pass. `upload_texture()` is implemented with proper transfer buffer management. Current limitation: only MSL (Metal) shaders are embedded — Vulkan/DirectX builds will throw at runtime until SPIR-V bytecode is added.

**Audio + Resampler**: the emulator runs at a native sample rate (e.g. 1 789 773 Hz for NES APU) and the `Resampler` converts to the output device rate (e.g. 44 100 Hz) using libsoxr. `set_out_rate()` nudges the output rate slightly each frame for DRC.

**Input** (`src/input.cpp`): processes SDL events to track connected gamepads. Falls back to keyboard if no gamepad is present — arrow keys for D-pad, Z/X for A/B, RShift/Enter for Select/Start.

**math3d** (`src/math3d.cpp`): column-major 4×4 matrix math (`Mat4`), right-handed coordinate system, Z mapped to [0, 1] (Metal/DX12 NDC). Provides perspective, look-at, and per-axis rotation helpers.

**`Span<T>`** is defined in `include/phosphene/span.h` as a minimal C++17 stand-in for `std::span`, used by `DrawCommand` in `renderer.h`.
