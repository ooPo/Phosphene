//
// resampler.cpp
// by Naomi Peori <naomi@peori.ca>
//

#include "phosphene/resampler.h"
#include <stdexcept>
#include <string>

void Resampler::init(double in_rate, double out_rate, int channels) {
  m_in_rate      = in_rate;
  m_nominal_rate = out_rate;
  m_channels     = channels;

  soxr_error_t err;
  soxr_quality_spec_t quality = soxr_quality_spec(SOXR_HQ, 0);
  soxr_io_spec_t io           = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);

  m_ctx = soxr_create(in_rate, out_rate, channels, &err, &io, &quality, nullptr);
  if (err) {
    throw std::runtime_error(std::string("soxr_create: ") + err);
  }
}

void Resampler::set_out_rate(double new_out_rate) {
  soxr_set_io_ratio(m_ctx, m_in_rate / new_out_rate, /*slew_len=*/0);
}

size_t Resampler::process(const float *in, size_t in_count,
                          float *out, size_t out_capacity) {
  size_t in_consumed = 0;
  size_t out_frames  = 0;

  soxr_error_t err = soxr_process(
      m_ctx,
      in, in_count / (size_t)m_channels, &in_consumed,
      out, out_capacity / (size_t)m_channels, &out_frames);

  if (err) {
    throw std::runtime_error(std::string("soxr_process: ") + err);
  }

  return out_frames * (size_t)m_channels;
}

void Resampler::shutdown() {
  if (m_ctx) {
    soxr_delete(m_ctx);
  }
  m_ctx = nullptr;
}
