// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <das/audio/Processor.h>
#include <das/transport/NamedSharedMemory.h>
#include <das/transport/RingBuffer.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <array>
#include <atomic>
#include <memory>

class MainComponent final : public juce::AudioAppComponent,
                            private juce::Timer {
public:
  MainComponent();
  ~MainComponent() override;

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
  void releaseResources() override;
  void paint(juce::Graphics& graphics) override;
  void resized() override;

private:
  void timerCallback() override;
  void showAudioSettings();

  das::transport::NamedSharedMemory memory_;
  std::unique_ptr<das::transport::RingBuffer> ring_;
  std::array<float, 16384> interleaved_ {};
  das::audio::SmoothedGain outputGain_;
  std::atomic<float> peak_ {};
  std::atomic<std::uint64_t> underflows_ {};
  double deviceSampleRate_ {};
  std::uint64_t lastProducerHeartbeat_ {};
  double lastProducerActivityMs_ {};

  juce::Label title_;
  juce::Label connectionStatus_;
  juce::Label deviceStatus_;
  juce::Label obsGuide_;
  juce::Slider gainSlider_;
  juce::Label gainLabel_;
  juce::TextButton settingsButton_;
  juce::TextButton obsButton_;
  juce::ProgressBar levelBar_;
  double displayedLevel_ {};
};
