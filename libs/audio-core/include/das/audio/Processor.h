// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <span>

namespace das::audio {

class SmoothedGain final {
public:
  void prepare(double sampleRate, double rampSeconds) noexcept;
  void reset(float gain = 1.0F) noexcept;
  void setTarget(float gain) noexcept;
  [[nodiscard]] float next() noexcept;
  void process(std::span<float> samples) noexcept;

private:
  float current_ {1.0F};
  float target_ {1.0F};
  float step_ {};
  std::size_t rampSamples_ {1};
  std::size_t remaining_ {};
};

class PeakMeter final {
public:
  void prepare(double sampleRate, double releaseSeconds) noexcept;
  [[nodiscard]] float process(std::span<const float> samples) noexcept;
  [[nodiscard]] float value() const noexcept { return value_; }
  void reset() noexcept { value_ = 0.0F; }

private:
  float value_ {};
  float releaseCoefficient_ {};
};

void softClip(std::span<float> samples, float drive = 1.0F) noexcept;

} // namespace das::audio
