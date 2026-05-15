//
// renderer.h
// by Naomi Peori <naomi@peori.ca>
//

#pragma once
#include "math3d.h"
#include "span.h"
#include "window.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include <cstring>
#include <vector>

/// @brief Per-vertex data for 3D geometry.
///
/// Covers the common case for PS1-style geometry:
/// transformed position, per-vertex colour, and a single set of UVs.
/// Extend with additional fields as needed (e.g. normals for lighting).
struct Vertex {
  float x, y, z, w; ///< Position (set w=1 for standard geometry)
  float r, g, b, a; ///< Per-vertex colour
  float u, v;       ///< Texture coordinates
};

/// @brief A single draw call for the 3D renderer.
///
/// Populate only what the draw needs; nullptr/zero fields are handled
/// gracefully in submit_draw(). The mvp array is column-major and defaults
/// to the identity transform.
struct DrawCommand {
  Span<const Vertex> vertices;
  Span<const uint16_t> indices;      ///< empty = non-indexed draw
  SDL_GPUTexture *texture = nullptr; ///< nullptr = untextured pipeline
  bool depth_test         = true;
  bool depth_write        = true;
  /// Enable src-alpha blending (output = src*srcA + dst*(1-srcA)).
  /// Also disables depth read and write — submit alpha-blended draws last
  /// so they composite correctly over opaque geometry.
  bool alpha_blend = false;
  Mat4 mvp         = Mat4::identity(); ///< Column-major MVP matrix (world→clip)
};

/// @brief 3D renderer for transformed geometry.
///
/// Renderer manages vertex buffers, pipelines, and render passes.
/// Typical usage: init(), begin_frame(), multiple submit_draw() calls,
/// end_frame(), repeat. Supports textured and untextured draws with
/// depth testing.
///
/// submit_draw() accumulates draw commands. end_frame() uploads all
/// geometry in a single copy pass, then issues the render pass.
/// Caller-owned vertex/index data must remain valid until end_frame()
/// returns.
class Renderer {
public:
  /// Initialize the 3D renderer.
  /// @param win Window (must remain valid for lifetime of Renderer)
  /// @param render_width Internal resolution width (e.g., 640 for PS1)
  /// @param render_height Internal resolution height (e.g., 480 for PS1)
  /// @throws std::runtime_error if initialization fails
  void init(Window &win, int render_width, int render_height);

  /// Start a new frame. Must be called before any submit_draw() calls.
  void begin_frame();

  /// Submit a draw call into the current frame.
  /// Must be called between begin_frame() and end_frame().
  void submit_draw(const DrawCommand &cmd);

  /// Upload all submitted draws and present to the screen.
  void end_frame();

  /// Clean up all GPU resources.
  void shutdown();

  // -- Texture management --

  /// Allocate a new GPU texture (RGBA8, sampler-usable).
  /// The returned pointer is owned by Renderer and must be released
  /// with destroy_texture().
  SDL_GPUTexture *create_texture(int width, int height);

  /// Release a texture previously allocated with create_texture().
  void destroy_texture(SDL_GPUTexture *texture);

  /// Upload RGBA8 pixel data to a texture created with create_texture().
  /// @param pitch Row stride in bytes (width * 4 for tightly-packed RGBA8).
  void upload_texture(SDL_GPUTexture *texture, const void *pixels, int pitch);

private:
  struct PendingDraw {
    Span<const Vertex> vertices;
    Span<const uint16_t> indices;
    SDL_GPUTexture *texture;
    bool depth_test;
    bool depth_write;
    bool alpha_blend;
    Mat4 mvp;
  };

  struct TexEntry {
    SDL_GPUTexture *tex;
    int w, h;
  };

  Window *m_ctx                                        = nullptr; // non-owning
  SDL_GPUTexture *m_depth_texture                      = nullptr;
  SDL_GPUSampler *m_sampler                            = nullptr;
  SDL_GPUGraphicsPipeline *m_pipeline_textured         = nullptr;
  SDL_GPUGraphicsPipeline *m_pipeline_untextured       = nullptr;
  SDL_GPUGraphicsPipeline *m_pipeline_textured_alpha   = nullptr;
  SDL_GPUGraphicsPipeline *m_pipeline_untextured_alpha = nullptr;
  SDL_GPUCommandBuffer *m_cmd                          = nullptr;
  SDL_GPUTexture *m_swapchain_tex                      = nullptr;
  int m_render_width                                   = 0;
  int m_render_height                                  = 0;

  SDL_GPUBuffer *m_vertex_buf = nullptr;
  SDL_GPUBuffer *m_index_buf  = nullptr;
  Uint32 m_vertex_buf_cap     = 0;
  Uint32 m_index_buf_cap      = 0;

  SDL_GPUTransferBuffer *m_geo_transfer_buf = nullptr;
  Uint32 m_geo_transfer_buf_cap             = 0;
  SDL_GPUTransferBuffer *m_tex_upload_buf   = nullptr;
  Uint32 m_tex_upload_buf_cap               = 0;

  std::vector<PendingDraw> m_pending_draws;
  std::vector<TexEntry> m_tex_entries;

  SDL_GPUGraphicsPipeline *create_pipeline(bool textured, bool alpha_blend = false);
  void ensure_vertex_buffer(Uint32 bytes);
  void ensure_index_buffer(Uint32 bytes);
  void ensure_geo_transfer_buffer(Uint32 bytes);
  void ensure_tex_upload_buffer(Uint32 bytes);
};
