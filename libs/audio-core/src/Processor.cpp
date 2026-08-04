// SPDX-License-Identifier: GPL-3.0-or-later
#include <das/audio/Processor.h>

#include <algorithm>
#include <cmath>

namespace das::audio {

void SmoothedGain::prepare(const double sampleRate, const double rampSeconds) noexcept {
  const auto count = std::llround(std::max(0.0, sampleRate * rampSeconds));
  rampSamples_ = static_cast<std::size_t>(std::max<long long>(1, count));
}

void SmoothedGain::reset(const float gain) noexcept {
  current_ = target_ = gain;
  step_ = 0.0F;
  remaining_ = 0;
}

void SmoothedGain::setTarget(const float gain) noexcept {
  target_ = gain;
  remaining_ = rampSamples_;
  step_ = (target_ - current_) / static_cast<float>(remaining_);
}

float SmoothedGain::next() noexcept {
  if (remaining_ != 0) {
    current_ += step_;
    if (--remaining_ == 0)
      current_ = target_;
  }
  return current_;
}

void SmoothedGain::process(const std::span<float> samples) noexcept {
  for (auto& sample : samples)
    sample *= next();
}

void PeakMeter::prepare(const double sampleRate, const double releaseSeconds) noexcept {
  if (sampleRate <= 0.0 || releaseSeconds <= 0.0) {
    releaseCoefficient_ = 0.0F;
    return;
  }
  releaseCoefficient_ = static_cast<float>(std::exp(-1.0 / (sampleRate * releaseSeconds)));
}

float PeakMeter::process(const std::span<const float> samples) noexcept {
  for (const auto sample : samples)
    value_ = std::max(std::abs(sample), value_ * releaseCoefficient_);
  return value_;
}

void softClip(const std::span<float> samples, const float drive) noexcept {
  const auto safeDrive = std::max(0.0001F, drive);
  const auto normalization = std::tanh(safeDrive);
  for (auto& sample : samples)
    sample = std::tanh(sample * safeDrive) / normalization;
}

} // namespace das::audio
