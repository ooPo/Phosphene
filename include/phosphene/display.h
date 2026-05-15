//
// display.h
// by Naomi Peori <naomi@peori.ca>
//

#pragma once
#include "window.h"
#include <SDL3/SDL.h>
#include <cstdint>

/// @brief Holds pixel data for a framebuffer.
struct Framebuffer {
  const uint32_t *pixels; ///< Pointer to pixel data (RGBA8)
  int width;              ///< Width in pixels
  int height;             ///< Height in pixels
};

/// @brief Simple 2D renderer for pixel-based systems.
///
/// Display uploads a framebuffer to a GPU texture each frame and blits
/// it to the window using nearest-neighbour filtering. This is typically
/// used for retro emulators (NES, SNES, Genesis, etc.).
class Display {
public:
  /// Initialize the 2D renderer.
  /// @param win Window (must remain valid for lifetime of Display)
  /// @param fb_width Framebuffer width in pixels
  /// @param fb_height Framebuffer height in pixels
  /// @throws std::runtime_error if texture creation fails
  void init(Window &win, int fb_width, int fb_height);

  /// Upload framebuffer and present to screen.
  /// The framebuffer is scaled to fit the current window size.
  void present(const Framebuffer &fb);

  /// Clean up GPU texture.
  void shutdown();

private:
  Window *m_win                         = nullptr; // non-owning
  SDL_GPUTexture *m_texture             = nullptr;
  SDL_GPUTransferBuffer *m_transfer_buf = nullptr;
  int m_fb_width                        = 0;
  int m_fb_height                       = 0;
};
