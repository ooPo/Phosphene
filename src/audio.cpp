//
// audio.cpp
// by Naomi Peori <naomi@peori.ca>
//

#include "phosphene/audio.h"
#include "phosphene/resampler.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

void Audio::init(int sample_rate, int channels, double emulated_fps) {
  m_sample_rate       = sample_rate;
  m_channels          = channels;
  m_samples_per_frame = (int)std::round(sample_rate / emulated_fps);

  SDL_AudioSpec spec = {};
  spec.format        = SDL_AUDIO_F32;
  spec.channels      = channels;
  spec.freq          = sample_rate;

  m_stream = SDL_OpenAudioDeviceStream(
      SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
  if (!m_stream) {
    throw std::runtime_error(std::string("SDL_OpenAudioDeviceStream: ") + SDL_GetError());
  }

  SDL_ResumeAudioStreamDevice(m_stream);
}

void Audio::push(const float *samples, int count) {
  if (samples && count > 0) {
    SDL_PutAudioStreamData(m_stream, samples, count * (int)sizeof(float));
  }
}

void Audio::wait_for_frame() const {
  const int target = m_samples_per_frame * 2;
  while (queued_samples() > target) {
    SDL_DelayNS(500 * 1000); // yield 500µs
  }
}

double Audio::compute_drc_rate(const Resampler &r) const {
  int error = queued_samples() - (m_samples_per_frame * 2);

  const double max_correction = 0.005;
  double correction           = (error / (double)m_samples_per_frame) * max_correction;
  correction                  = std::clamp(correction, -max_correction, max_correction);

  return r.nominal_rate() * (1.0 + correction);
}

void Audio::shutdown() {
  if (m_stream) {
    SDL_DestroyAudioStream(m_stream);
  }
  m_stream = nullptr;
}

int Audio::queued_samples() const {
  return SDL_GetAudioStreamQueued(m_stream) / (int)(sizeof(float) * (size_t)m_channels);
}
