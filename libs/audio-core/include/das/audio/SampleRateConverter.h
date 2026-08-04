// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace das::audio {

// Streaming, band-limited stereo sample-rate conversion for the distribution
// path. All allocations and filter construction happen in prepare().
class SampleRateConverter final {
public:
  SampleRateConverter();
  ~SampleRateConverter();

  SampleRateConverter(const SampleRateConverter&) = delete;
  SampleRateConverter& operator=(const SampleRateConverter&) = delete;

  bool prepare(double inputSampleRate, double outputSampleRate,
               std::size_t maximumInputFrames);
  void reset() noexcept;

  // Writes interleaved stereo samples and returns the number of output frames.
  // Mono callers may pass the same span for left and right.
  [[nodiscard]] std::uint32_t process(std::span<const float> left,
                                      std::span<const float> right,
                                      std::span<float> interleavedOutput) noexcept;

  [[nodiscard]] std::size_t maximumInputFrames() const noexcept;
  [[nodiscard]] std::size_t maximumOutputFrames() const noexcept;
  [[nodiscard]] bool isPrepared() const noexcept;
  [[nodiscard]] bool isBypassed() const noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace das::audio
