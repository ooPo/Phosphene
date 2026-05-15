//
// window.cpp
// by Naomi Peori <naomi@peori.ca>
//

#include "phosphene/window.h"
#include <stdexcept>
#include <string>

void Window::init(const char *title, int width, int height) {
  m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
  if (!m_window) {
    throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());
  }

  // Pass all known shader formats — SDL3 will select the best one for the
  // current platform automatically (MSL on macOS, SPIRV/DXIL on others).
  m_gpu = SDL_CreateGPUDevice(
      SDL_GPU_SHADERFORMAT_SPIRV |
          SDL_GPU_SHADERFORMAT_MSL |
          SDL_GPU_SHADERFORMAT_DXIL,
      true, nullptr);
  if (!m_gpu) {
    throw std::runtime_error(std::string("SDL_CreateGPUDevice: ") + SDL_GetError());
  }

  SDL_ClaimWindowForGPUDevice(m_gpu, m_window);

  // Prefer MAILBOX (tear-free, non-blocking) but fall back to VSYNC if the
  // driver or Metal configuration doesn't support it.
  if (!SDL_SetGPUSwapchainParameters(
          m_gpu, m_window,
          SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
          SDL_GPU_PRESENTMODE_MAILBOX)) {
    SDL_Log("MAILBOX present mode unavailable, falling back to VSYNC");
    SDL_SetGPUSwapchainParameters(
        m_gpu, m_window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        SDL_GPU_PRESENTMODE_VSYNC);
  }
}

void Window::shutdown() {
  // Order matters: release the device before destroying the window
  if (m_gpu) {
    SDL_DestroyGPUDevice(m_gpu);
  }
  if (m_window) {
    SDL_DestroyWindow(m_window);
  }
  m_gpu    = nullptr;
  m_window = nullptr;
}
