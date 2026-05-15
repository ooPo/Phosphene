//
// renderer.cpp
// by Naomi Peori <naomi@peori.ca>
//

#include "phosphene/renderer.h"
#include <cstring>
#include <stdexcept>
#include <string>

// ============================================================
//  Embedded shader source (MSL for Metal/macOS)
//
//  For Vulkan (Linux/Windows) pre-compiled SPIR-V byte arrays would
//  be selected instead. See the SDL3 GPU shader binding docs for
//  the required resource set layout.
//
//  MSL binding layout (SDL3 GPU graphics shaders):
//    Vertex stage:   [[buffer(0)]] = uniform slot 0 (SDL_PushGPUVertexUniformData)
//                    [[stage_in]]  = vertex attributes (auto from pipeline desc)
//    Fragment stage: [[texture(0)]], [[sampler(0)]] = texture/sampler slot 0
// ============================================================

static const char *const k_shader_msl = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float4 position [[attribute(0)]];
    float4 color    [[attribute(1)]];
    float2 uv       [[attribute(2)]];
};

struct Varyings {
    float4 position [[position]];
    float4 color;
    float2 uv;
};

vertex Varyings vs_main(
    VertexIn in [[stage_in]],
    constant float4x4& mvp [[buffer(0)]]
) {
    Varyings out;
    out.position = mvp * float4(in.position.xyz, 1.0);
    out.color    = in.color;
    out.uv       = in.uv;
    return out;
}

fragment float4 fs_color(Varyings in [[stage_in]]) {
    return in.color;
}

fragment float4 fs_textured(
    Varyings in [[stage_in]],
    texture2d<float> tex [[texture(0)]],
    sampler          smp [[sampler(0)]]
) {
    return tex.sample(smp, in.uv) * in.color;
}
)MSL";

// ============================================================
//  Shader loading
// ============================================================

static SDL_GPUShader *load_shader(
    SDL_GPUDevice *gpu,
    SDL_GPUShaderStage stage,
    const char *entrypoint,
    Uint32 num_uniform_buffers,
    Uint32 num_samplers) {
  SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(gpu);

  SDL_GPUShaderCreateInfo info = {};
  info.stage                   = stage;
  info.entrypoint              = entrypoint;
  info.num_uniform_buffers     = num_uniform_buffers;
  info.num_samplers            = num_samplers;

  if (formats & SDL_GPU_SHADERFORMAT_MSL) {
    info.format    = SDL_GPU_SHADERFORMAT_MSL;
    info.code      = (const Uint8 *)k_shader_msl;
    info.code_size = SDL_strlen(k_shader_msl);
  } else {
    throw std::runtime_error(
        "Renderer: only MSL (Metal) shaders are currently embedded. "
        "Add pre-compiled SPIR-V bytecode for Vulkan support.");
  }

  SDL_GPUShader *shader = SDL_CreateGPUShader(gpu, &info);
  if (!shader) {
    throw std::runtime_error(std::string("load_shader '") + entrypoint + "': " + SDL_GetError());
  }
  return shader;
}

// ============================================================
//  Pipeline creation
// ============================================================

SDL_GPUGraphicsPipeline *Renderer::create_pipeline(bool textured, bool alpha_blend) {
  SDL_GPUDevice *gpu = m_ctx->gpu();

  SDL_GPUShader *vert = load_shader(gpu, SDL_GPU_SHADERSTAGE_VERTEX,
                                    "vs_main", /*num_uniform_buffers=*/1, /*num_samplers=*/0);

  SDL_GPUShader *frag = load_shader(gpu, SDL_GPU_SHADERSTAGE_FRAGMENT,
                                    textured ? "fs_textured" : "fs_color",
                                    /*num_uniform_buffers=*/0,
                                    /*num_samplers=*/textured ? 1 : 0);

  // Vertex layout matches struct Vertex: float4 pos, float4 color, float2 uv
  SDL_GPUVertexAttribute attrs[3] = {};
  attrs[0]                        = {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, (Uint32)offsetof(Vertex, x)};
  attrs[1]                        = {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, (Uint32)offsetof(Vertex, r)};
  attrs[2]                        = {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, (Uint32)offsetof(Vertex, u)};

  SDL_GPUVertexBufferDescription vbd = {};
  vbd.slot                           = 0;
  vbd.pitch                          = sizeof(Vertex);
  vbd.input_rate                     = SDL_GPU_VERTEXINPUTRATE_VERTEX;
  vbd.instance_step_rate             = 0;

  SDL_GPUVertexInputState vert_input    = {};
  vert_input.vertex_buffer_descriptions = &vbd;
  vert_input.num_vertex_buffers         = 1;
  vert_input.vertex_attributes          = attrs;
  vert_input.num_vertex_attributes      = 3;

  SDL_GPUColorTargetDescription color_desc = {};
  color_desc.format                        = SDL_GetGPUSwapchainTextureFormat(gpu, m_ctx->window());

  if (alpha_blend) {
    color_desc.blend_state.enable_blend          = true;
    color_desc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    color_desc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_desc.blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    color_desc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color_desc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    color_desc.blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
  }

  SDL_GPUGraphicsPipelineCreateInfo pinfo = {};
  pinfo.vertex_shader                     = vert;
  pinfo.fragment_shader                   = frag;
  pinfo.vertex_input_state                = vert_input;
  pinfo.primitive_type                    = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

  pinfo.rasterizer_state.fill_mode  = SDL_GPU_FILLMODE_FILL;
  pinfo.rasterizer_state.cull_mode  = SDL_GPU_CULLMODE_BACK;
  pinfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

  pinfo.depth_stencil_state.enable_depth_test  = !alpha_blend;
  pinfo.depth_stencil_state.enable_depth_write = !alpha_blend;
  pinfo.depth_stencil_state.compare_op         = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

  pinfo.target_info.color_target_descriptions = &color_desc;
  pinfo.target_info.num_color_targets         = 1;
  pinfo.target_info.depth_stencil_format      = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
  pinfo.target_info.has_depth_stencil_target  = true;

  SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pinfo);

  SDL_ReleaseGPUShader(gpu, vert);
  SDL_ReleaseGPUShader(gpu, frag);

  if (!pipeline) {
    throw std::runtime_error(std::string("create_pipeline: ") + SDL_GetError());
  }
  return pipeline;
}

// ============================================================
//  GPU buffer helpers
// ============================================================

void Renderer::ensure_vertex_buffer(Uint32 bytes) {
  if (bytes <= m_vertex_buf_cap) {
    return;
  }
  if (m_vertex_buf) {
    SDL_ReleaseGPUBuffer(m_ctx->gpu(), m_vertex_buf);
  }
  Uint32 alloc = m_vertex_buf_cap ? m_vertex_buf_cap * 2 : bytes;
  while (alloc < bytes) alloc *= 2;
  SDL_GPUBufferCreateInfo info = {};
  info.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
  info.size                    = alloc;
  m_vertex_buf                 = SDL_CreateGPUBuffer(m_ctx->gpu(), &info);
  if (!m_vertex_buf) {
    throw std::runtime_error(std::string("vertex buffer: ") + SDL_GetError());
  }
  m_vertex_buf_cap = alloc;
}

void Renderer::ensure_index_buffer(Uint32 bytes) {
  if (bytes <= m_index_buf_cap) {
    return;
  }
  if (m_index_buf) {
    SDL_ReleaseGPUBuffer(m_ctx->gpu(), m_index_buf);
  }
  Uint32 alloc = m_index_buf_cap ? m_index_buf_cap * 2 : bytes;
  while (alloc < bytes) alloc *= 2;
  SDL_GPUBufferCreateInfo info = {};
  info.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
  info.size                    = alloc;
  m_index_buf                  = SDL_CreateGPUBuffer(m_ctx->gpu(), &info);
  if (!m_index_buf) {
    throw std::runtime_error(std::string("index buffer: ") + SDL_GetError());
  }
  m_index_buf_cap = alloc;
}

void Renderer::ensure_geo_transfer_buffer(Uint32 bytes) {
  if (bytes <= m_geo_transfer_buf_cap) {
    return;
  }
  if (m_geo_transfer_buf) {
    SDL_ReleaseGPUTransferBuffer(m_ctx->gpu(), m_geo_transfer_buf);
  }
  Uint32 alloc = m_geo_transfer_buf_cap ? m_geo_transfer_buf_cap * 2 : bytes;
  while (alloc < bytes) alloc *= 2;
  SDL_GPUTransferBufferCreateInfo info = {};
  info.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  info.size                            = alloc;
  m_geo_transfer_buf                   = SDL_CreateGPUTransferBuffer(m_ctx->gpu(), &info);
  if (!m_geo_transfer_buf) {
    throw std::runtime_error(std::string("geo transfer buffer: ") + SDL_GetError());
  }
  m_geo_transfer_buf_cap = alloc;
}

void Renderer::ensure_tex_upload_buffer(Uint32 bytes) {
  if (bytes <= m_tex_upload_buf_cap) {
    return;
  }
  if (m_tex_upload_buf) {
    SDL_ReleaseGPUTransferBuffer(m_ctx->gpu(), m_tex_upload_buf);
  }
  Uint32 alloc = m_tex_upload_buf_cap ? m_tex_upload_buf_cap * 2 : bytes;
  while (alloc < bytes) alloc *= 2;
  SDL_GPUTransferBufferCreateInfo info = {};
  info.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  info.size                            = alloc;
  m_tex_upload_buf                     = SDL_CreateGPUTransferBuffer(m_ctx->gpu(), &info);
  if (!m_tex_upload_buf) {
    throw std::runtime_error(std::string("tex upload buffer: ") + SDL_GetError());
  }
  m_tex_upload_buf_cap = alloc;
}

// ============================================================
//  Init / Shutdown
// ============================================================

void Renderer::init(Window &ctx, int render_width, int render_height) {
  m_ctx           = &ctx;
  m_render_width  = render_width;
  m_render_height = render_height;

  // -- Depth texture --
  SDL_GPUTextureCreateInfo depth_info = {};
  depth_info.type                     = SDL_GPU_TEXTURETYPE_2D;
  depth_info.format                   = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
  depth_info.usage                    = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
  depth_info.width                    = (Uint32)render_width;
  depth_info.height                   = (Uint32)render_height;
  depth_info.layer_count_or_depth     = 1;
  depth_info.num_levels               = 1;

  m_depth_texture = SDL_CreateGPUTexture(m_ctx->gpu(), &depth_info);
  if (!m_depth_texture) {
    throw std::runtime_error(std::string("depth texture: ") + SDL_GetError());
  }

  // -- Default sampler (nearest, clamp) --
  SDL_GPUSamplerCreateInfo sampler_info = {};
  sampler_info.min_filter               = SDL_GPU_FILTER_NEAREST;
  sampler_info.mag_filter               = SDL_GPU_FILTER_NEAREST;
  sampler_info.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
  sampler_info.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  sampler_info.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  sampler_info.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

  m_sampler = SDL_CreateGPUSampler(m_ctx->gpu(), &sampler_info);
  if (!m_sampler) {
    throw std::runtime_error(std::string("sampler: ") + SDL_GetError());
  }

  // -- Pipelines --
  m_pipeline_untextured       = create_pipeline(/*textured=*/false);
  m_pipeline_textured         = create_pipeline(/*textured=*/true);
  m_pipeline_untextured_alpha = create_pipeline(/*textured=*/false, /*alpha_blend=*/true);
  m_pipeline_textured_alpha   = create_pipeline(/*textured=*/true, /*alpha_blend=*/true);
}

void Renderer::shutdown() {
  if (m_pipeline_textured) {
    SDL_ReleaseGPUGraphicsPipeline(m_ctx->gpu(), m_pipeline_textured);
  }
  if (m_pipeline_untextured) {
    SDL_ReleaseGPUGraphicsPipeline(m_ctx->gpu(), m_pipeline_untextured);
  }
  if (m_pipeline_textured_alpha) {
    SDL_ReleaseGPUGraphicsPipeline(m_ctx->gpu(), m_pipeline_textured_alpha);
  }
  if (m_pipeline_untextured_alpha) {
    SDL_ReleaseGPUGraphicsPipeline(m_ctx->gpu(), m_pipeline_untextured_alpha);
  }
  if (m_vertex_buf) {
    SDL_ReleaseGPUBuffer(m_ctx->gpu(), m_vertex_buf);
  }
  if (m_index_buf) {
    SDL_ReleaseGPUBuffer(m_ctx->gpu(), m_index_buf);
  }
  if (m_geo_transfer_buf) {
    SDL_ReleaseGPUTransferBuffer(m_ctx->gpu(), m_geo_transfer_buf);
  }
  if (m_tex_upload_buf) {
    SDL_ReleaseGPUTransferBuffer(m_ctx->gpu(), m_tex_upload_buf);
  }
  if (m_depth_texture) {
    SDL_ReleaseGPUTexture(m_ctx->gpu(), m_depth_texture);
  }
  if (m_sampler) {
    SDL_ReleaseGPUSampler(m_ctx->gpu(), m_sampler);
  }

  m_pipeline_textured         = nullptr;
  m_pipeline_untextured       = nullptr;
  m_pipeline_textured_alpha   = nullptr;
  m_pipeline_untextured_alpha = nullptr;
  m_vertex_buf                = nullptr;
  m_index_buf                 = nullptr;
  m_vertex_buf_cap            = 0;
  m_index_buf_cap             = 0;
  m_geo_transfer_buf          = nullptr;
  m_geo_transfer_buf_cap      = 0;
  m_tex_upload_buf            = nullptr;
  m_tex_upload_buf_cap        = 0;
  m_depth_texture             = nullptr;
  m_sampler                   = nullptr;
  m_ctx                       = nullptr;
  m_pending_draws.clear();
  m_tex_entries.clear();
}

// ============================================================
//  Frame lifecycle
// ============================================================

void Renderer::begin_frame() {
  m_pending_draws.clear();

  m_cmd = SDL_AcquireGPUCommandBuffer(m_ctx->gpu());
  if (!m_cmd) {
    throw std::runtime_error(std::string("begin_frame AcquireCommandBuffer: ") + SDL_GetError());
  }

  Uint32 sw_w = 0, sw_h = 0;
  SDL_WaitAndAcquireGPUSwapchainTexture(
      m_cmd, m_ctx->window(), &m_swapchain_tex, &sw_w, &sw_h);

  if (!m_swapchain_tex) {
    return; // minimised or not ready
  }

  // Recreate depth texture if window was resized
  if ((int)sw_w != m_render_width || (int)sw_h != m_render_height) {
    SDL_ReleaseGPUTexture(m_ctx->gpu(), m_depth_texture);

    SDL_GPUTextureCreateInfo di = {};
    di.type                     = SDL_GPU_TEXTURETYPE_2D;
    di.format                   = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    di.usage                    = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    di.width                    = sw_w;
    di.height                   = sw_h;
    di.layer_count_or_depth     = 1;
    di.num_levels               = 1;

    m_depth_texture = SDL_CreateGPUTexture(m_ctx->gpu(), &di);
    if (!m_depth_texture) {
      throw std::runtime_error(std::string("depth texture resize: ") + SDL_GetError());
    }

    m_render_width  = (int)sw_w;
    m_render_height = (int)sw_h;
  }
}

void Renderer::submit_draw(const DrawCommand &cmd) {
  if (!m_swapchain_tex) {
    return; // frame was skipped
  }

  PendingDraw d;
  d.vertices    = cmd.vertices;
  d.indices     = cmd.indices;
  d.texture     = cmd.texture;
  d.depth_test  = cmd.depth_test;
  d.depth_write = cmd.depth_write;
  d.alpha_blend = cmd.alpha_blend;
  d.mvp         = cmd.mvp;
  m_pending_draws.push_back(d);
}

void Renderer::end_frame() {
  if (!m_swapchain_tex) {
    // Window minimised — still submit the empty command buffer
    if (m_cmd) {
      SDL_SubmitGPUCommandBuffer(m_cmd);
      m_cmd = nullptr;
    }
    return;
  }

  // ---- 1. Upload all geometry in a single copy pass ----

  Uint32 total_verts   = 0;
  Uint32 total_indices = 0;
  for (auto &d : m_pending_draws) {
    total_verts += (Uint32)d.vertices.size;
    total_indices += (Uint32)d.indices.size;
  }

  if (total_verts > 0) {
    Uint32 vbytes = total_verts * (Uint32)sizeof(Vertex);
    Uint32 ibytes = total_indices * (Uint32)sizeof(Uint16);

    ensure_vertex_buffer(vbytes);
    if (ibytes > 0) {
      ensure_index_buffer(ibytes);
    }

    ensure_geo_transfer_buffer(vbytes + ibytes);
    auto *mapped = (Uint8 *)SDL_MapGPUTransferBuffer(m_ctx->gpu(), m_geo_transfer_buf, false);

    // Pack vertices
    Uint32 vo = 0;
    for (auto &d : m_pending_draws) {
      Uint32 sz = (Uint32)(d.vertices.size * sizeof(Vertex));
      memcpy(mapped + vo, d.vertices.data, sz);
      vo += sz;
    }
    // Pack indices (after vertices)
    Uint32 io = vbytes;
    for (auto &d : m_pending_draws) {
      if (!d.indices.empty()) {
        Uint32 sz = (Uint32)(d.indices.size * sizeof(Uint16));
        memcpy(mapped + io, d.indices.data, sz);
        io += sz;
      }
    }

    SDL_UnmapGPUTransferBuffer(m_ctx->gpu(), m_geo_transfer_buf);

    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(m_cmd);
    {
      SDL_GPUTransferBufferLocation src = {m_geo_transfer_buf, 0};
      SDL_GPUBufferRegion dst           = {m_vertex_buf, 0, vbytes};
      SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    }
    if (ibytes > 0) {
      SDL_GPUTransferBufferLocation src = {m_geo_transfer_buf, vbytes};
      SDL_GPUBufferRegion dst           = {m_index_buf, 0, ibytes};
      SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    }
    SDL_EndGPUCopyPass(copy);
  }

  // ---- 2. Render pass ----

  SDL_GPUColorTargetInfo color_target = {};
  color_target.texture                = m_swapchain_tex;
  color_target.load_op                = SDL_GPU_LOADOP_CLEAR;
  color_target.store_op               = SDL_GPU_STOREOP_STORE;
  color_target.clear_color            = {0.1f, 0.1f, 0.15f, 1.0f};

  SDL_GPUDepthStencilTargetInfo depth_target = {};
  depth_target.texture                       = m_depth_texture;
  depth_target.load_op                       = SDL_GPU_LOADOP_CLEAR;
  depth_target.store_op                      = SDL_GPU_STOREOP_DONT_CARE;
  depth_target.clear_depth                   = 1.0f;

  SDL_GPURenderPass *rp = SDL_BeginGPURenderPass(m_cmd, &color_target, 1, &depth_target);

  SDL_GPUViewport vp = {};
  vp.w               = (float)m_render_width;
  vp.h               = (float)m_render_height;
  vp.min_depth       = 0.0f;
  vp.max_depth       = 1.0f;
  SDL_SetGPUViewport(rp, &vp);

  Uint32 vert_byte_off = 0;
  Uint32 idx_byte_off  = 0;
  for (auto &d : m_pending_draws) {
    if (d.vertices.size == 0) {
      continue;
    }

    SDL_GPUGraphicsPipeline *pipeline;
    if (d.alpha_blend) {
      pipeline = d.texture ? m_pipeline_textured_alpha : m_pipeline_untextured_alpha;
    } else {
      pipeline = d.texture ? m_pipeline_textured : m_pipeline_untextured;
    }
    SDL_BindGPUGraphicsPipeline(rp, pipeline);

    // Push MVP matrix to vertex uniform slot 0
    SDL_PushGPUVertexUniformData(m_cmd, 0, d.mvp.data(), 16 * sizeof(float));

    SDL_GPUBufferBinding vbind = {m_vertex_buf, vert_byte_off};
    SDL_BindGPUVertexBuffers(rp, 0, &vbind, 1);

    if (d.texture) {
      SDL_GPUTextureSamplerBinding tb = {d.texture, m_sampler};
      SDL_BindGPUFragmentSamplers(rp, 0, &tb, 1);
    }

    if (!d.indices.empty()) {
      SDL_GPUBufferBinding ibind = {m_index_buf, idx_byte_off};
      SDL_BindGPUIndexBuffer(rp, &ibind, SDL_GPU_INDEXELEMENTSIZE_16BIT);
      SDL_DrawGPUIndexedPrimitives(rp, (Uint32)d.indices.size, 1, 0, 0, 0);
      idx_byte_off += (Uint32)(d.indices.size * sizeof(Uint16));
    } else {
      SDL_DrawGPUPrimitives(rp, (Uint32)d.vertices.size, 1, 0, 0);
    }

    vert_byte_off += (Uint32)(d.vertices.size * sizeof(Vertex));
  }

  SDL_EndGPURenderPass(rp);
  SDL_SubmitGPUCommandBuffer(m_cmd);
  m_cmd           = nullptr;
  m_swapchain_tex = nullptr;
  m_pending_draws.clear();
}

// ============================================================
//  Texture management
// ============================================================

SDL_GPUTexture *Renderer::create_texture(int width, int height) {
  SDL_GPUTextureCreateInfo info = {};
  info.type                     = SDL_GPU_TEXTURETYPE_2D;
  info.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
  info.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
  info.width                    = (Uint32)width;
  info.height                   = (Uint32)height;
  info.layer_count_or_depth     = 1;
  info.num_levels               = 1;

  SDL_GPUTexture *tex = SDL_CreateGPUTexture(m_ctx->gpu(), &info);
  if (!tex) {
    throw std::runtime_error(std::string("create_texture: ") + SDL_GetError());
  }

  m_tex_entries.push_back({tex, width, height});
  return tex;
}

void Renderer::destroy_texture(SDL_GPUTexture *tex) {
  if (!tex) {
    return;
  }
  for (auto it = m_tex_entries.begin(); it != m_tex_entries.end(); ++it) {
    if (it->tex == tex) {
      m_tex_entries.erase(it);
      break;
    }
  }
  SDL_ReleaseGPUTexture(m_ctx->gpu(), tex);
}

void Renderer::upload_texture(SDL_GPUTexture *texture, const void *pixels, int pitch) {
  // Look up stored dimensions
  int width = 0, height = 0;
  for (auto &e : m_tex_entries) {
    if (e.tex == texture) {
      width  = e.w;
      height = e.h;
      break;
    }
  }
  if (width == 0 || height == 0) {
    throw std::runtime_error("upload_texture: texture was not created by this Renderer");
  }

  Uint32 upload_size = (Uint32)(pitch * height);

  ensure_tex_upload_buffer(upload_size);
  // cycle=true: if the GPU is still reading from a previous upload, the
  // driver provides a fresh backing allocation rather than stalling.
  void *mapped = SDL_MapGPUTransferBuffer(m_ctx->gpu(), m_tex_upload_buf, true);
  memcpy(mapped, pixels, upload_size);
  SDL_UnmapGPUTransferBuffer(m_ctx->gpu(), m_tex_upload_buf);

  SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(m_ctx->gpu());
  SDL_GPUCopyPass *copy     = SDL_BeginGPUCopyPass(cmd);

  SDL_GPUTextureTransferInfo src = {};
  src.transfer_buffer            = m_tex_upload_buf;
  src.offset                     = 0;
  src.pixels_per_row             = (Uint32)(pitch / 4); // RGBA8: 4 bytes per pixel
  src.rows_per_layer             = (Uint32)height;

  SDL_GPUTextureRegion dst = {};
  dst.texture              = texture;
  dst.w                    = (Uint32)width;
  dst.h                    = (Uint32)height;
  dst.d                    = 1;

  SDL_UploadToGPUTexture(copy, &src, &dst, false);
  SDL_EndGPUCopyPass(copy);
  SDL_SubmitGPUCommandBuffer(cmd);
}
