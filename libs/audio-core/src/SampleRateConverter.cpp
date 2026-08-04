// SPDX-License-Identifier: GPL-3.0-or-later
#include <das/audio/SampleRateConverter.h>

#include <CDSPResampler.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace das::audio {

struct SampleRateConverter::Impl final {
  bool prepare(const double newInputRate, const double newOutputRate,
               const std::size_t requestedMaximumInputFrames) {
    if (!std::isfinite(newInputRate) || !std::isfinite(newOutputRate) ||
        newInputRate <= 0.0 || newOutputRate <= 0.0 ||
        requestedMaximumInputFrames == 0 ||
        requestedMaximumInputFrames > static_cast<std::size_t>(std::numeric_limits<int>::max()))
      return false;

    inputRate = newInputRate;
    outputRate = newOutputRate;
    maximumInput = requestedMaximumInputFrames;
    bypassed = std::abs(inputRate - outputRate) < 0.001;

    for (auto& buffer : input)
      buffer.assign(maximumInput, 0.0);

    if (bypassed) {
      maximumOutput = maximumInput;
      for (auto& converter : converters) converter.reset();
    } else {
      for (auto& converter : converters)
        converter = std::make_unique<r8b::CDSPResampler24>(
            inputRate, outputRate, static_cast<int>(maximumInput), 3.5);
      maximumOutput = static_cast<std::size_t>(std::max(
          converters[0]->getMaxOutLen(static_cast<int>(maximumInput)), 1));
    }
    prepared = true;
    return true;
  }

  void reset() noexcept {
    for (auto& converter : converters)
      if (converter) converter->clear();
  }

  std::array<std::unique_ptr<r8b::CDSPResampler24>, 2> converters;
  std::array<std::vector<double>, 2> input;
  std::size_t maximumInput {};
  std::size_t maximumOutput {};
  double inputRate {};
  double outputRate {};
  bool prepared {};
  bool bypassed {};
};

SampleRateConverter::SampleRateConverter() : impl_(std::make_unique<Impl>()) {}
SampleRateConverter::~SampleRateConverter() = default;

bool SampleRateConverter::prepare(const double inputSampleRate,
                                  const double outputSampleRate,
                                  const std::size_t maximumInputFrames) {
  return impl_->prepare(inputSampleRate, outputSampleRate, maximumInputFrames);
}

void SampleRateConverter::reset() noexcept { impl_->reset(); }

std::uint32_t SampleRateConverter::process(const std::span<const float> left,
                                           const std::span<const float> right,
                                           const std::span<float> interleavedOutput) noexcept {
  if (!impl_->prepared || left.empty() || right.size() < left.size() ||
      left.size() > impl_->maximumInput)
    return 0;

  if (impl_->bypassed) {
    const auto frames = std::min(left.size(), interleavedOutput.size() / 2);
    for (std::size_t frame = 0; frame < frames; ++frame) {
      interleavedOutput[frame * 2] = left[frame];
      interleavedOutput[frame * 2 + 1] = right[frame];
    }
    return static_cast<std::uint32_t>(frames);
  }

  for (std::size_t frame = 0; frame < left.size(); ++frame) {
    impl_->input[0][frame] = static_cast<double>(left[frame]);
    impl_->input[1][frame] = static_cast<double>(right[frame]);
  }

  std::array<double*, 2> output {};
  std::array<int, 2> outputFrames {};
  for (std::size_t channel = 0; channel < output.size(); ++channel)
    outputFrames[channel] = impl_->converters[channel]->process(
        impl_->input[channel].data(), static_cast<int>(left.size()), output[channel]);

  const auto frames = static_cast<std::size_t>(std::max(
      0, std::min(outputFrames[0], outputFrames[1])));
  if (frames > impl_->maximumOutput || interleavedOutput.size() < frames * 2)
    return 0;

  for (std::size_t frame = 0; frame < frames; ++frame) {
    interleavedOutput[frame * 2] = static_cast<float>(output[0][frame]);
    interleavedOutput[frame * 2 + 1] = static_cast<float>(output[1][frame]);
  }
  return static_cast<std::uint32_t>(frames);
}

std::size_t SampleRateConverter::maximumInputFrames() const noexcept {
  return impl_->maximumInput;
}

std::size_t SampleRateConverter::maximumOutputFrames() const noexcept {
  return impl_->maximumOutput;
}

bool SampleRateConverter::isPrepared() const noexcept { return impl_->prepared; }
bool SampleRateConverter::isBypassed() const noexcept { return impl_->bypassed; }

} // namespace das::audio
