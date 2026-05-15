//
// main.cpp
// by Naomi Peori <naomi@peori.ca>
//

#include "phosphene/input.h"
#include "phosphene/math3d.h"
#include "phosphene/renderer.h"
#include "phosphene/window.h"
#include <SDL3/SDL.h>

// ============================================================
//  Cube geometry  (24 vertices: 4 per face × 6 faces)
//
//  Winding: CCW when viewed from outside (back-face culled).
//  Vertex colours are white so the texture colour is unmodified
//  by the per-vertex tint (shader does tex.sample() * color).
// ============================================================

// clang-format off
static const Vertex cube_verts[24] = {
    // Front  (+Z)
    { -0.5f, -0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f },
    {  0.5f, -0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f },
    {  0.5f,  0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f },
    { -0.5f,  0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f },
    // Back   (-Z)
    {  0.5f, -0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f },
    { -0.5f, -0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f },
    { -0.5f,  0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f },
    {  0.5f,  0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f },
    // Left   (-X)
    { -0.5f, -0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f },
    { -0.5f, -0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f },
    { -0.5f,  0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f },
    { -0.5f,  0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f },
    // Right  (+X)
    {  0.5f, -0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f },
    {  0.5f, -0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f },
    {  0.5f,  0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f },
    {  0.5f,  0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f },
    // Top    (+Y)
    { -0.5f,  0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f },
    {  0.5f,  0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f },
    {  0.5f,  0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f },
    { -0.5f,  0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f },
    // Bottom (-Y)
    { -0.5f, -0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f },
    {  0.5f, -0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f },
    {  0.5f, -0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f },
    { -0.5f, -0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f },
};

static const uint16_t cube_indices[36] = {
     0,  1,  2,   0,  2,  3,  // front
     4,  5,  6,   4,  6,  7,  // back
     8,  9, 10,   8, 10, 11,  // left
    12, 13, 14,  12, 14, 15,  // right
    16, 17, 18,  16, 18, 19,  // top
    20, 21, 22,  20, 22, 23,  // bottom
};
// clang-format on

// ============================================================
//  Procedural checkerboard texture
// ============================================================

static void make_checkerboard(uint32_t *pixels, int w, int h, int cell) {
  // RGBA8 — 0xAABBGGRR in little-endian memory (SDL ABGR layout)
  const uint32_t light = 0xFFD4B483; // warm sand
  const uint32_t dark  = 0xFF3D6B8A; // muted teal
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      pixels[y * w + x] = ((x / cell + y / cell) & 1) ? dark : light;
    }
  }
}

// ============================================================
//  Main
// ============================================================

int main() {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD);

  const int WIN_W = 800;
  const int WIN_H = 600;

  Window ctx;
  Renderer renderer;
  Input input;

  ctx.init("Textured Cube", WIN_W, WIN_H);
  renderer.init(ctx, WIN_W, WIN_H);
  input.init();

  // Build and upload a 64×64 procedural texture
  const int TEX_SIZE = 64;
  const int CELL     = 8;
  static uint32_t tex_pixels[TEX_SIZE * TEX_SIZE];
  make_checkerboard(tex_pixels, TEX_SIZE, TEX_SIZE, CELL);

  SDL_GPUTexture *tex = renderer.create_texture(TEX_SIZE, TEX_SIZE);
  renderer.upload_texture(tex, tex_pixels, TEX_SIZE * 4);

  // Fixed camera and projection
  Mat4 view = math3d_look_at({0.0f, 1.2f, 3.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
  Mat4 proj = math3d_perspective(3.14159265f / 4.0f,
                                 (float)WIN_W / (float)WIN_H, 0.1f, 100.0f);

  float angle_y  = 0.0f;
  float angle_x  = 0.35f;
  float angle_z  = 0.0f;
  Uint64 last_ns = SDL_GetTicksNS();
  bool running   = true;
  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        proj = math3d_perspective(3.14159265f / 4.0f,
                                  (float)event.window.data1 / (float)event.window.data2,
                                  0.1f, 100.0f);
      }
      input.handle_event(event);
    }

    Uint64 now = SDL_GetTicksNS();
    float dt   = (float)((double)(now - last_ns) * 1e-9);
    last_ns    = now;
    if (dt > 0.1f) {
      dt = 0.1f;
    }

    angle_y += 0.8f * dt;
    angle_z += 0.25f * dt;

    Mat4 model = math3d_rotation_z(angle_z) * math3d_rotation_x(angle_x) * math3d_rotation_y(angle_y);
    Mat4 mvp   = proj * view * model;

    DrawCommand cmd;
    cmd.vertices = {cube_verts, 24};
    cmd.indices  = {cube_indices, 36};
    cmd.texture  = tex;
    cmd.mvp      = mvp;

    renderer.begin_frame();
    renderer.submit_draw(cmd);
    renderer.end_frame();
  }

  renderer.destroy_texture(tex);
  input.shutdown();
  renderer.shutdown();
  ctx.shutdown();
  SDL_Quit();
  return 0;
}
