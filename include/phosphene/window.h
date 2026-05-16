//
// window.h
// by Naomi Peori <naomi@peori.ca>
//

#pragma once
#include <SDL3/SDL.h>

/// @brief Manages SDL3 GPU device and window lifecycle.
///
/// Window handles the core SDL3 initialization, GPU device creation,
/// and window management. It is typically initialized early and shared
/// with other components like FramebufferDisplay and Video3D.
class Window {
public:
  /// Initialize SDL3 GPU context and create a window.
  /// @param title Window title
  /// @param width Window width in pixels
  /// @param height Window height in pixels
  /// @throws std::runtime_error if SDL3 initialization fails
  void init(const char *title, int width, int height);

  /// Clean up GPU device and window resources.
  void shutdown();

  /// Get the GPU device handle (non-null after init).
  SDL_GPUDevice *gpu() const { return m_gpu; }

  /// Get the window handle (non-null after init).
  SDL_Window *window() const { return m_window; }

  /// Returns true when the swapchain is using VSYNC present mode.
  /// Use this to skip audio-based frame pacing (wait_for_frame()), since VSYNC
  /// already provides it — running both causes double-blocking and slowdowns.
  bool is_vsync() const { return m_present_mode == SDL_GPU_PRESENTMODE_VSYNC; }

private:
  SDL_GPUDevice    *m_gpu          = nullptr;
  SDL_Window       *m_window       = nullptr;
  SDL_GPUPresentMode m_present_mode = SDL_GPU_PRESENTMODE_VSYNC;
};
