//
// display.cpp
// by Naomi Peori <naomi@peori.ca>
//

#include "phosphene/display.h"
#include <cstring>
#include <stdexcept>
#include <string>

void Display::init(Window &win, int fb_width, int fb_height) {
  m_win       = &win;
  m_fb_width  = fb_width;
  m_fb_height = fb_height;

  SDL_GPUTextureCreateInfo tex_info = {};
  tex_info.type                     = SDL_GPU_TEXTURETYPE_2D;
  tex_info.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
  tex_info.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
  tex_info.width                    = (uint32_t)fb_width;
  tex_info.height                   = (uint32_t)fb_height;
  tex_info.layer_count_or_depth     = 1;
  tex_info.num_levels               = 1;

  m_texture = SDL_CreateGPUTexture(m_win->gpu(), &tex_info);
  if (!m_texture) {
    throw std::runtime_error(std::string("SDL_CreateGPUTexture: ") + SDL_GetError());
  }

  SDL_GPUTransferBufferCreateInfo tbuf_info = {};
  tbuf_info.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  tbuf_info.size                            = (Uint32)(fb_width * fb_height * 4);
  m_transfer_buf = SDL_CreateGPUTransferBuffer(m_win->gpu(), &tbuf_info);
  if (!m_transfer_buf) {
    throw std::runtime_error(std::string("SDL_CreateGPUTransferBuffer: ") + SDL_GetError());
  }
}

void Display::present(const Framebuffer &fb) {
  if (fb.width != m_fb_width || fb.height != m_fb_height) {
    throw std::runtime_error(
        "Display::present: framebuffer dimensions don't match init dimensions");
  }

  size_t fb_bytes = (size_t)(fb.width * fb.height * 4);

  void *mapped = SDL_MapGPUTransferBuffer(m_win->gpu(), m_transfer_buf, false);
  memcpy(mapped, fb.pixels, fb_bytes);
  SDL_UnmapGPUTransferBuffer(m_win->gpu(), m_transfer_buf);

  SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(m_win->gpu());

  // Copy transfer buffer into the GPU texture
  SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);

  SDL_GPUTextureTransferInfo src = {};
  src.transfer_buffer            = m_transfer_buf;
  src.pixels_per_row             = (uint32_t)fb.width; // pixels, not bytes

  SDL_GPUTextureRegion dst_region = {};
  dst_region.texture              = m_texture;
  dst_region.w                    = (uint32_t)fb.width;
  dst_region.h                    = (uint32_t)fb.height;
  dst_region.d                    = 1;

  SDL_UploadToGPUTexture(copy, &src, &dst_region, false);
  SDL_EndGPUCopyPass(copy);

  // Blit to swapchain
  SDL_GPUTexture *swapchain_tex = nullptr;
  SDL_WaitAndAcquireGPUSwapchainTexture(
      cmd, m_win->window(), &swapchain_tex, nullptr, nullptr);

  if (swapchain_tex) {
    int win_w = 0, win_h = 0;
    SDL_GetWindowSizeInPixels(m_win->window(), &win_w, &win_h);

    SDL_GPUBlitInfo blit     = {};
    blit.source.texture      = m_texture;
    blit.source.w            = (uint32_t)fb.width;
    blit.source.h            = (uint32_t)fb.height;
    blit.destination.texture = swapchain_tex;
    blit.destination.w       = (uint32_t)win_w;
    blit.destination.h       = (uint32_t)win_h;
    blit.load_op             = SDL_GPU_LOADOP_DONT_CARE;
    blit.filter              = SDL_GPU_FILTER_NEAREST;

    SDL_BlitGPUTexture(cmd, &blit);
  }

  SDL_SubmitGPUCommandBuffer(cmd);
}

void Display::shutdown() {
  if (m_transfer_buf) {
    SDL_ReleaseGPUTransferBuffer(m_win->gpu(), m_transfer_buf);
  }
  if (m_texture) {
    SDL_ReleaseGPUTexture(m_win->gpu(), m_texture);
  }
  m_transfer_buf = nullptr;
  m_texture      = nullptr;
  m_win          = nullptr;
}
