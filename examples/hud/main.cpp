//
// main.cpp
// by Naomi Peori <naomi@peori.ca>
//

#include "phosphene/input.h"
#include "phosphene/math3d.h"
#include "phosphene/renderer.h"
#include "phosphene/window.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <cstring>

// ============================================================
//  Cube geometry  (24 vertices: 4 per face × 6 faces)
//  Same winding and layout as the textured_cube example.
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
//  HUD quad: full-screen clip-space quad with identity MVP.
//
//  Vertices are already in NDC (no projection needed).
//  UV (0,0) = texture top-left = screen top-left.
//  Submitted with alpha_blend=true so transparent pixels
//  in the HUD texture show through to the 3D scene below.
// ============================================================

// clang-format off
static const Vertex hud_verts[4] = {
    // pos (clip space)           colour (white = no tint)   UV
    { -1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f }, // BL
    {  1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f }, // BR
    {  1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f }, // TR
    { -1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f }, // TL
};

static const uint16_t hud_indices[6] = { 0, 1, 2,  0, 2, 3 };
// clang-format on

// ============================================================
//  Checkerboard texture for the cube
// ============================================================

static void make_checkerboard(uint32_t *pixels, int w, int h, int cell) {
  // RGBA8: stored as 0xAABBGGRR in little-endian uint32
  const uint32_t light = 0xFFD4B483;
  const uint32_t dark  = 0xFF3D6B8A;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      pixels[y * w + x] = ((x / cell + y / cell) & 1) ? dark : light;
    }
  }
}

// ============================================================
//  HUD texture generation
//
//  256×192 RGBA8 texture; most pixels are transparent.
//  Elements drawn each frame to animate the health bar.
// ============================================================

// Pack RGBA into the 0xAABBGGRR uint32 layout used by R8G8B8A8_UNORM
static constexpr uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
  return (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(g) << 8) | uint32_t(r);
}

static const uint32_t HUD_TRANSPARENT = rgba(0, 0, 0, 0);
static const uint32_t HUD_PANEL       = rgba(10, 10, 18, 180); // dark navy, ~70% opaque
static const uint32_t HUD_WHITE       = rgba(255, 255, 255);
static const uint32_t HUD_GREEN       = rgba(50, 210, 80);      // health fill
static const uint32_t HUD_EMPTY       = rgba(110, 10, 20);      // health empty
static const uint32_t HUD_CYAN        = rgba(0, 210, 220, 200); // crosshair, ~78% opaque
static const uint32_t HUD_YELLOW      = rgba(255, 210, 0);      // score panel border
static const uint32_t HUD_ORANGE      = rgba(255, 140, 0, 220); // corner brackets, ~86% opaque

static void set_px(uint32_t *p, int w, int h, int x, int y, uint32_t c) {
  if (x >= 0 && x < w && y >= 0 && y < h) {
    p[y * w + x] = c;
  }
}

static void fill_rect(uint32_t *p, int w, int h, int rx, int ry, int rw, int rh, uint32_t c) {
  for (int y = ry; y < ry + rh; ++y) {
    for (int x = rx; x < rx + rw; ++x) {
      set_px(p, w, h, x, y, c);
    }
  }
}

static void outline_rect(uint32_t *p, int w, int h, int rx, int ry, int rw, int rh, uint32_t c) {
  for (int x = rx; x < rx + rw; ++x) {
    set_px(p, w, h, x, ry, c);
    set_px(p, w, h, x, ry + rh - 1, c);
  }
  for (int y = ry; y < ry + rh; ++y) {
    set_px(p, w, h, rx, y, c);
    set_px(p, w, h, rx + rw - 1, y, c);
  }
}

// L-shaped corner bracket: corner at (cx,cy), arms extend in (dx,0) and (0,dy)
static void draw_bracket(uint32_t *p, int w, int h,
                         int cx, int cy, int dx, int dy, int arm, uint32_t c) {
  for (int i = 0; i < arm; ++i) {
    set_px(p, w, h, cx + dx * i, cy, c);
    set_px(p, w, h, cx, cy + dy * i, c);
  }
}

static void make_hud(uint32_t *pixels, int tw, int th, float health) {
  for (int i = 0; i < tw * th; ++i) {
    pixels[i] = HUD_TRANSPARENT;
  }

  // ---- Health bar (bottom-left) ----
  fill_rect(pixels, tw, th, 4, th - 22, 108, 18, HUD_PANEL);
  fill_rect(pixels, tw, th, 8, th - 18, 100, 8, HUD_EMPTY);
  int fill_px = (int)(100.0f * health + 0.5f);
  if (fill_px > 100) {
    fill_px = 100;
  }
  if (fill_px > 0) {
    fill_rect(pixels, tw, th, 8, th - 18, fill_px, 8, HUD_GREEN);
  }
  outline_rect(pixels, tw, th, 7, th - 19, 102, 10, HUD_WHITE);

  // ---- Score panel (top-right) ----
  fill_rect(pixels, tw, th, tw - 68, 4, 64, 14, HUD_PANEL);
  outline_rect(pixels, tw, th, tw - 69, 3, 66, 16, HUD_YELLOW);

  // ---- Corner brackets (all four corners) ----
  const int ARM = 12, PAD = 4;
  draw_bracket(pixels, tw, th, PAD, PAD, +1, +1, ARM, HUD_ORANGE);
  draw_bracket(pixels, tw, th, tw - 1 - PAD, PAD, -1, +1, ARM, HUD_ORANGE);
  draw_bracket(pixels, tw, th, PAD, th - 1 - PAD, +1, -1, ARM, HUD_ORANGE);
  draw_bracket(pixels, tw, th, tw - 1 - PAD, th - 1 - PAD, -1, -1, ARM, HUD_ORANGE);

  // ---- Crosshair (center) ----
  const int cx = tw / 2, cy = th / 2;
  const int GAP = 3, LEN = 10;
  for (int i = GAP + 1; i <= GAP + LEN; ++i) {
    set_px(pixels, tw, th, cx + i, cy, HUD_CYAN);
    set_px(pixels, tw, th, cx - i, cy, HUD_CYAN);
    set_px(pixels, tw, th, cx, cy + i, HUD_CYAN);
    set_px(pixels, tw, th, cx, cy - i, HUD_CYAN);
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

  ctx.init("HUD Overlay", WIN_W, WIN_H);
  renderer.init(ctx, WIN_W, WIN_H);
  input.init();

  // ---- Cube texture ----
  const int TEX_SIZE = 64, CELL = 8;
  static uint32_t cube_pixels[TEX_SIZE * TEX_SIZE];
  make_checkerboard(cube_pixels, TEX_SIZE, TEX_SIZE, CELL);
  SDL_GPUTexture *cube_tex = renderer.create_texture(TEX_SIZE, TEX_SIZE);
  renderer.upload_texture(cube_tex, cube_pixels, TEX_SIZE * 4);

  // ---- HUD texture (256×192 — scales to 800×600 at exactly 3.125×) ----
  const int HUD_W = 256, HUD_H = 192;
  static uint32_t hud_pixels[HUD_W * HUD_H];
  SDL_GPUTexture *hud_tex = renderer.create_texture(HUD_W, HUD_H);

  // ---- Camera / projection ----
  Mat4 view = math3d_look_at({0.0f, 1.2f, 3.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
  Mat4 proj = math3d_perspective(3.14159265f / 4.0f,
                                 (float)WIN_W / (float)WIN_H, 0.1f, 100.0f);

  float angle_y  = 0.0f;
  float angle_x  = 0.35f;
  float angle_z  = 0.0f;
  float time     = 0.0f;
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

    time += dt;
    angle_y += 0.8f * dt;
    angle_z += 0.25f * dt;

    // Animate health between ~10% and ~90%
    float health = 0.5f + 0.4f * std::sin(time * 0.8f);

    // Re-generate and upload the HUD texture each frame
    make_hud(hud_pixels, HUD_W, HUD_H, health);
    renderer.upload_texture(hud_tex, hud_pixels, HUD_W * 4);

    // ---- 3D scene: spinning textured cube ----
    Mat4 model = math3d_rotation_z(angle_z) * math3d_rotation_x(angle_x) * math3d_rotation_y(angle_y);
    Mat4 mvp   = proj * view * model;

    DrawCommand cube_cmd;
    cube_cmd.vertices = {cube_verts, 24};
    cube_cmd.indices  = {cube_indices, 36};
    cube_cmd.texture  = cube_tex;
    cube_cmd.mvp      = mvp;

    // ---- HUD overlay: alpha-blended full-screen quad ----
    // Submitted after the cube so it composites on top.
    // alpha_blend=true selects the blending pipeline and disables
    // depth read/write — the HUD always draws over the 3D scene.
    DrawCommand hud_cmd;
    hud_cmd.vertices    = {hud_verts, 4};
    hud_cmd.indices     = {hud_indices, 6};
    hud_cmd.texture     = hud_tex;
    hud_cmd.alpha_blend = true;
    hud_cmd.mvp         = Mat4::identity();

    renderer.begin_frame();
    renderer.submit_draw(cube_cmd);
    renderer.submit_draw(hud_cmd);
    renderer.end_frame();
  }

  renderer.destroy_texture(hud_tex);
  renderer.destroy_texture(cube_tex);
  input.shutdown();
  renderer.shutdown();
  ctx.shutdown();
  SDL_Quit();
  return 0;
}
