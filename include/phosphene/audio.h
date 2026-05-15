//
// audio.h
// by Naomi Peori <naomi@peori.ca>
//

#pragma once
#include <SDL3/SDL.h>

// Forward declared to avoid pulling soxr.h into every translation unit
// that only needs to know about Audio
class Resampler;

/// @brief Manages SDL3 audio streaming with frame-based synchronization.
///
/// Audio provides sample rate conversion and frame-pacing utilities.
/// Samples are pushed continuously and internally buffered; the caller
/// may query the queue depth or wait until a target level is reached
/// using wait_for_frame().
class Audio {
public:
  /// Initialize audio device and queue.
  /// @param sample_rate Output sample rate (e.g., 44100)
  /// @param channels Number of channels (typically 1 or 2)
  /// @param emulated_fps Target frame rate for frame-based pacing
  /// @throws std::runtime_error if audio device cannot be opened
  void init(int sample_rate, int channels, double emulated_fps);

  /// Enqueue audio samples into the playback queue.
  /// @param samples Pointer to float samples
  /// @param count Number of samples to enqueue
  void push(const float *samples, int count);

  /// Block until the audio queue reaches a target level.
  /// Used for frame pacing in emulators.
  void wait_for_frame() const;

  /// Compute a rate adjustment for dynamic rate correction (DRC).
  /// Returns the adjusted sample rate based on buffer fullness.
  double compute_drc_rate(const Resampler &r) const;

  /// Clean up audio device and stream.
  void shutdown();

  int sample_rate() const { return m_sample_rate; }
  int samples_per_frame() const { return m_samples_per_frame; }

private:
  SDL_AudioStream *m_stream = nullptr;
  int m_sample_rate         = 0;
  int m_channels            = 0;
  int m_samples_per_frame   = 0;

  int queued_samples() const;
};
