//
// main.cpp
// by Naomi Peori <naomi@peori.ca>
//

#include "phosphene/audio.h"
#include "phosphene/display.h"
#include "phosphene/input.h"
#include "phosphene/resampler.h"
#include "phosphene/window.h"
#include <SDL3/SDL.h>
#include <cmath>

// ============================================================
//  Colour cycling
// ============================================================

static uint32_t hsv_to_rgba(float h, float s, float v) {
  float c = v * s;
  float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
  float m = v - c;
  float r, g, b;

  if (h < 60) {
    r = c;
    g = x;
    b = 0;
  } else if (h < 120) {
    r = x;
    g = c;
    b = 0;
  } else if (h < 180) {
    r = 0;
    g = c;
    b = x;
  } else if (h < 240) {
    r = 0;
    g = x;
    b = c;
  } else if (h < 300) {
    r = x;
    g = 0;
    b = c;
  } else {
    r = c;
    g = 0;
    b = x;
  }

  uint8_t R = (uint8_t)((r + m) * 255.0f);
  uint8_t G = (uint8_t)((g + m) * 255.0f);
  uint8_t B = (uint8_t)((b + m) * 255.0f);
  return (uint32_t)R | ((uint32_t)G << 8) | ((uint32_t)B << 16) | (0xFFu << 24);
}

static void fill_framebuffer(uint32_t *pixels, int count, float hue) {
  uint32_t colour = hsv_to_rgba(hue, 0.85f, 0.9f);
  for (int i = 0; i < count; ++i) {
    pixels[i] = colour;
  }
}

// ============================================================
//  Sine wave generator
// ============================================================

struct SineGen {
  double phase     = 0.0;
  double phase2    = 0.0;
  double freq1     = 440.0;
  double freq2     = 660.0;
  double amplitude = 0.25;
};

static int fill_sine(SineGen &gen, float *out, int count, double sample_rate) {
  const double phase_inc1 = (2.0 * M_PI * gen.freq1) / sample_rate;
  const double phase_inc2 = (2.0 * M_PI * gen.freq2) / sample_rate;

  for (int i = 0; i < count; ++i) {
    float s = (float)((sin(gen.phase) * gen.amplitude) +
                      (sin(gen.phase2) * gen.amplitude));
    out[i]  = s;
    gen.phase += phase_inc1;
    gen.phase2 += phase_inc2;
  }

  gen.phase  = fmod(gen.phase, 2.0 * M_PI);
  gen.phase2 = fmod(gen.phase2, 2.0 * M_PI);

  return count;
}

// ============================================================
//  Main
// ============================================================

int main() {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);

  // NES constants
  const double NES_APU_RATE       = 1789773.0;
  const double NES_FPS            = 60.098;
  const int OUTPUT_RATE           = 44100;
  const int NES_FB_W              = 256;
  const int NES_FB_H              = 240;
  const int APU_SAMPLES_PER_FRAME = 29830;
  const int MAX_OUTPUT_SAMPLES    = 1024;
  const int DEFAULT_SCALE         = 2;

  Window ctx;
  Display video;
  Audio audio;
  Resampler resampler;
  Input input;

  ctx.init("Emulator Frontend — Example",
           NES_FB_W * DEFAULT_SCALE,
           NES_FB_H * DEFAULT_SCALE);
  video.init(ctx, NES_FB_W, NES_FB_H);
  audio.init(OUTPUT_RATE, /*channels=*/1, NES_FPS);
  resampler.init(NES_APU_RATE, OUTPUT_RATE, /*channels=*/1);
  input.init();

  static uint32_t framebuffer[NES_FB_W * NES_FB_H];
  static float apu_buf[APU_SAMPLES_PER_FRAME];
  float out_buf[MAX_OUTPUT_SAMPLES];

  SineGen sine;
  float hue                 = 0.0f;
  const float hue_per_frame = 360.0f / (float)(NES_FPS * 10.0);

  bool running = true;
  SDL_Event event;

  while (running) {
    // 1. Events
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }

      // Number keys 1–5 set the integer scale of the window
      if (event.type == SDL_EVENT_KEY_DOWN) {
        int scale = 0;
        switch (event.key.scancode) {
        case SDL_SCANCODE_1:
          scale = 1;
          break;
        case SDL_SCANCODE_2:
          scale = 2;
          break;
        case SDL_SCANCODE_3:
          scale = 3;
          break;
        case SDL_SCANCODE_4:
          scale = 4;
          break;
        case SDL_SCANCODE_5:
          scale = 5;
          break;
        default:
          break;
        }
        if (scale > 0) {
          SDL_SetWindowSize(ctx.window(),
                            NES_FB_W * scale,
                            NES_FB_H * scale);
        }
      }

      input.handle_event(event);
    }

    // 2. Input
    InputState state = input.read();
    (void)state;

    // 3. Frame pacing
    audio.wait_for_frame();

    // 4. DRC
    double drc_rate = audio.compute_drc_rate(resampler);
    resampler.set_out_rate(drc_rate);

    // 5. Generate video
    fill_framebuffer(framebuffer, NES_FB_W * NES_FB_H, hue);
    hue = fmodf(hue + hue_per_frame, 360.0f);

    // 6. Generate audio
    fill_sine(sine, apu_buf, APU_SAMPLES_PER_FRAME, NES_APU_RATE);

    // 7. Resample
    size_t out_count = resampler.process(
        apu_buf, APU_SAMPLES_PER_FRAME,
        out_buf, MAX_OUTPUT_SAMPLES);

    // 8. Push audio and present video
    audio.push(out_buf, (int)out_count);

    const Framebuffer fb = {framebuffer, NES_FB_W, NES_FB_H};
    video.present(fb);
  }

  input.shutdown();
  resampler.shutdown();
  audio.shutdown();
  video.shutdown();
  ctx.shutdown();
  SDL_Quit();
  return 0;
}
