// SPDX-License-Identifier: GPL-3.0-or-later
#include <das/audio/Processor.h>
#include <das/audio/SampleRateConverter.h>

#include <array>
#include <cmath>
#include <iostream>
#include <numbers>
#include <vector>

namespace {
int failures {};
void check(const bool condition, const char* message) {
  if (!condition) { std::cerr << message << '\n'; ++failures; }
}

std::vector<float> resampleTone(const double inputRate, const double outputRate,
                                const double frequency, const double amplitude,
                                const int seconds) {
  constexpr std::size_t blockFrames = 1024;
  das::audio::SampleRateConverter converter;
  if (!converter.prepare(inputRate, outputRate, blockFrames)) return {};
  std::vector<float> left(blockFrames);
  std::vector<float> right(blockFrames);
  std::vector<float> output(converter.maximumOutputFrames() * 2);
  std::vector<float> collected;
  collected.reserve(static_cast<std::size_t>(outputRate * seconds) * 2);
  const auto totalInput = static_cast<std::size_t>(inputRate * seconds);
  std::size_t position {};
  while (position < totalInput) {
    const auto frames = std::min(blockFrames, totalInput - position);
    for (std::size_t frame = 0; frame < frames; ++frame) {
      const auto phase = 2.0 * std::numbers::pi * frequency *
                         static_cast<double>(position + frame) / inputRate;
      left[frame] = right[frame] = static_cast<float>(amplitude * std::sin(phase));
    }
    const auto produced = converter.process(
        std::span<const float>(left.data(), frames),
        std::span<const float>(right.data(), frames), output);
    collected.insert(collected.end(), output.begin(), output.begin() + produced * 2);
    position += frames;
  }
  return collected;
}

double channelRms(const std::vector<float>& interleaved, const double sampleRate,
                  const double skipSeconds = 0.25) {
  const auto firstFrame = std::min(interleaved.size() / 2,
      static_cast<std::size_t>(sampleRate * skipSeconds));
  double sum {};
  std::size_t count {};
  for (std::size_t frame = firstFrame; frame < interleaved.size() / 2; ++frame) {
    const auto value = static_cast<double>(interleaved[frame * 2]);
    sum += value * value;
    ++count;
  }
  return count == 0 ? 0.0 : std::sqrt(sum / static_cast<double>(count));
}
}

int main() {
  das::audio::SmoothedGain gain;
  gain.prepare(1000.0, 0.004);
  gain.reset(0.0F);
  gain.setTarget(1.0F);
  std::array<float, 4> samples {1, 1, 1, 1};
  gain.process(samples);
  check(std::abs(samples[0] - 0.25F) < 0.0001F, "ゲインランプの開始値が不正です");
  check(std::abs(samples[3] - 1.0F) < 0.0001F, "ゲインランプが目標値に到達しません");

  das::audio::PeakMeter meter;
  meter.prepare(48000.0, 0.3);
  const std::array<float, 3> peaks {0.1F, -0.8F, 0.2F};
  check(meter.process(peaks) >= 0.79F, "ピークメーターが負方向のピークを検出できません");

  std::array<float, 3> clipped {-4.0F, 0.0F, 4.0F};
  das::audio::softClip(clipped);
  check(clipped[0] >= -1.32F && clipped[2] <= 1.32F, "ソフトクリップの出力が範囲外です");
  check(clipped[1] == 0.0F, "ソフトクリップがゼロを変化させています");

  das::audio::SampleRateConverter bypass;
  check(bypass.prepare(48000.0, 48000.0, 8), "48 kHzバイパスを準備できません");
  const std::array<float, 4> bypassLeft {0.1F, -0.2F, 0.3F, -0.4F};
  const std::array<float, 4> bypassRight {-0.4F, 0.3F, -0.2F, 0.1F};
  std::array<float, 8> bypassOutput {};
  check(bypass.process(bypassLeft, bypassRight, bypassOutput) == 4,
        "48 kHzバイパスのフレーム数が不正です");
  for (std::size_t frame = 0; frame < bypassLeft.size(); ++frame) {
    check(bypassOutput[frame * 2] == bypassLeft[frame] &&
          bypassOutput[frame * 2 + 1] == bypassRight[frame],
          "48 kHzバイパスがサンプルを変更しています");
  }

  const auto audibleTone = resampleTone(96000.0, 48000.0, 10000.0, 0.5, 2);
  const auto ultrasonicTone = resampleTone(96000.0, 48000.0, 30000.0, 0.5, 2);
  const auto audibleRms = channelRms(audibleTone, 48000.0);
  const auto ultrasonicRms = channelRms(ultrasonicTone, 48000.0);
  check(audibleRms > 0.32 && audibleRms < 0.38,
        "96→48 kHz変換が可聴帯域のレベルを維持できません");
  check(ultrasonicRms < 0.0001,
        "96→48 kHz変換でナイキスト超過成分を十分に除去できません");

  const auto upsampledTone = resampleTone(44100.0, 48000.0, 1000.0, 0.5, 2);
  const auto highTone = resampleTone(44100.0, 48000.0, 20000.0, 0.5, 2);
  const auto upsampledRms = channelRms(upsampledTone, 48000.0);
  const auto highToneRms = channelRms(highTone, 48000.0);
  check(upsampledRms > 0.34 && upsampledRms < 0.37,
        "44.1→48 kHz変換のレベルが不正です");
  check(highToneRms > 0.32 && highToneRms < 0.38,
        "44.1→48 kHz変換で20 kHzの可聴高域が失われています");
  return failures == 0 ? 0 : 1;
}
