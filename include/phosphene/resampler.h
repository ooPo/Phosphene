//
// resampler.h
// by Naomi Peori <naomi@peori.ca>
//

#pragma once
#include <cstddef>
#include <soxr.h>

/// @brief Wraps libsoxr for high-quality audio resampling.
///
/// The Resampler converts audio between two sample rates. It supports
/// dynamic rate adjustment via set_out_rate(), enabling DRC (Dynamic
/// Rate Correction) for frame-perfect audio in emulators.
class Resampler {
public:
  /// Initialize resampler.
  /// @param in_rate Input sample rate (e.g., 1789773 for NES APU)
  /// @param out_rate Output sample rate (e.g., 44100)
  /// @param channels Number of audio channels
  /// @throws std::runtime_error if soxr initialization fails
  void init(double in_rate, double out_rate, int channels);

  /// Adjust the output sample rate (for DRC).
  void set_out_rate(double new_out_rate);

  /// Process audio samples.
  /// @param in Input samples (all channels interleaved)
  /// @param in_count Total number of input samples (frames × channels)
  /// @param out Output buffer
  /// @param out_capacity Output buffer capacity in total samples (frames × channels)
  /// @return Total number of output samples written (frames × channels)
  size_t process(const float *in, size_t in_count,
                 float *out, size_t out_capacity);

  /// Clean up resampler.
  void shutdown();

  /// Get the nominal output sample rate.
  double nominal_rate() const { return m_nominal_rate; }

private:
  soxr_t m_ctx          = nullptr;
  double m_in_rate      = 0.0;
  double m_nominal_rate = 0.0;
  int    m_channels     = 0;
};
