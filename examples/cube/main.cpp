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
//  Winding: CCW when viewed from outside the cube (back-face culled).
//  Each face has a distinct colour for easy visual debugging.
// ============================================================

// clang-format off
static const Vertex cube_verts[24] = {
    // Front  (+Z) — red
    { -0.5f, -0.5f,  0.5f, 1.0f,  1.0f, 0.2f, 0.2f, 1.0f,  0.0f, 1.0f },
    {  0.5f, -0.5f,  0.5f, 1.0f,  1.0f, 0.2f, 0.2f, 1.0f,  1.0f, 1.0f },
    {  0.5f,  0.5f,  0.5f, 1.0f,  1.0f, 0.2f, 0.2f, 1.0f,  1.0f, 0.0f },
    { -0.5f,  0.5f,  0.5f, 1.0f,  1.0f, 0.2f, 0.2f, 1.0f,  0.0f, 0.0f },
    // Back   (-Z) — green
    {  0.5f, -0.5f, -0.5f, 1.0f,  0.2f, 1.0f, 0.2f, 1.0f,  0.0f, 1.0f },
    { -0.5f, -0.5f, -0.5f, 1.0f,  0.2f, 1.0f, 0.2f, 1.0f,  1.0f, 1.0f },
    { -0.5f,  0.5f, -0.5f, 1.0f,  0.2f, 1.0f, 0.2f, 1.0f,  1.0f, 0.0f },
    {  0.5f,  0.5f, -0.5f, 1.0f,  0.2f, 1.0f, 0.2f, 1.0f,  0.0f, 0.0f },
    // Left   (-X) — blue
    { -0.5f, -0.5f, -0.5f, 1.0f,  0.2f, 0.2f, 1.0f, 1.0f,  0.0f, 1.0f },
    { -0.5f, -0.5f,  0.5f, 1.0f,  0.2f, 0.2f, 1.0f, 1.0f,  1.0f, 1.0f },
    { -0.5f,  0.5f,  0.5f, 1.0f,  0.2f, 0.2f, 1.0f, 1.0f,  1.0f, 0.0f },
    { -0.5f,  0.5f, -0.5f, 1.0f,  0.2f, 0.2f, 1.0f, 1.0f,  0.0f, 0.0f },
    // Right  (+X) — yellow
    {  0.5f, -0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 0.2f, 1.0f,  0.0f, 1.0f },
    {  0.5f, -0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 0.2f, 1.0f,  1.0f, 1.0f },
    {  0.5f,  0.5f, -0.5f, 1.0f,  1.0f, 1.0f, 0.2f, 1.0f,  1.0f, 0.0f },
    {  0.5f,  0.5f,  0.5f, 1.0f,  1.0f, 1.0f, 0.2f, 1.0f,  0.0f, 0.0f },
    // Top    (+Y) — cyan
    { -0.5f,  0.5f,  0.5f, 1.0f,  0.2f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f },
    {  0.5f,  0.5f,  0.5f, 1.0f,  0.2f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f },
    {  0.5f,  0.5f, -0.5f, 1.0f,  0.2f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f },
    { -0.5f,  0.5f, -0.5f, 1.0f,  0.2f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f },
    // Bottom (-Y) — magenta
    { -0.5f, -0.5f, -0.5f, 1.0f,  1.0f, 0.2f, 1.0f, 1.0f,  0.0f, 1.0f },
    {  0.5f, -0.5f, -0.5f, 1.0f,  1.0f, 0.2f, 1.0f, 1.0f,  1.0f, 1.0f },
    {  0.5f, -0.5f,  0.5f, 1.0f,  1.0f, 0.2f, 1.0f, 1.0f,  1.0f, 0.0f },
    { -0.5f, -0.5f,  0.5f, 1.0f,  1.0f, 0.2f, 1.0f, 1.0f,  0.0f, 0.0f },
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
//  Main
// ============================================================

int main() {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD);

  const int WIN_W = 800;
  const int WIN_H = 600;

  Window ctx;
  Renderer renderer;
  Input input;

  ctx.init("Spinning Cube", WIN_W, WIN_H);
  renderer.init(ctx, WIN_W, WIN_H);
  input.init();

  // Fixed camera and projection — recomputed only when aspect changes
  Mat4 view = math3d_look_at({0.0f, 1.2f, 3.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
  Mat4 proj = math3d_perspective(3.14159265f / 4.0f,
                                 (float)WIN_W / (float)WIN_H, 0.1f, 100.0f);

  float angle_y  = 0.0f;  // radians
  float angle_x  = 0.35f; // slight downward tilt, radians (fixed)
  float angle_z  = 0.0f;  // radians
  Uint64 last_ns = SDL_GetTicksNS();
  bool running   = true;
  SDL_Event event;

  while (running) {
    // Events
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

    // Delta time
    Uint64 now = SDL_GetTicksNS();
    float dt   = (float)((double)(now - last_ns) * 1e-9);
    last_ns    = now;
    if (dt > 0.1f) {
      dt = 0.1f; // clamp on focus loss / first frame
    }

    angle_y += 0.8f * dt;  // 0.8 rad/s spin
    angle_z += 0.25f * dt; // 0.25 rad/s slow roll

    // Build MVP: proj * view * rot_z * rot_x * rot_y
    Mat4 model = math3d_rotation_z(angle_z) * math3d_rotation_x(angle_x) * math3d_rotation_y(angle_y);
    Mat4 mvp   = proj * view * model;

    // Submit cube draw
    DrawCommand cmd;
    cmd.vertices = {cube_verts, 24};
    cmd.indices  = {cube_indices, 36};
    cmd.mvp      = mvp;

    renderer.begin_frame();
    renderer.submit_draw(cmd);
    renderer.end_frame();
  }

  input.shutdown();
  renderer.shutdown();
  ctx.shutdown();
  SDL_Quit();
  return 0;
}
