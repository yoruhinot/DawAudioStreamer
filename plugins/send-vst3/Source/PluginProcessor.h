// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include "DiscordBridge.h"

#include <das/audio/SampleRateConverter.h>
#include <das/transport/NamedSharedMemory.h>
#include <das/transport/RingBuffer.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

class DasSendProcessor final : public juce::AudioProcessor {
public:
  DasSendProcessor();
  ~DasSendProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return JucePlugin_Name; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}
  void getStateInformation(juce::MemoryBlock&) override {}
  void setStateInformation(const void*, int) override {}
  [[nodiscard]] double currentSampleRate() const noexcept { return sampleRate_.load(); }
  [[nodiscard]] bool sampleRateSupported() const noexcept { return sampleRateSupported_.load(); }
  [[nodiscard]] bool transportReady() const noexcept { return transportReady_.load(); }
  [[nodiscard]] bool isBypassed() const noexcept { return bypassed_.load(); }
  [[nodiscard]] bool isPrimarySender() const noexcept { return primarySender_; }
  [[nodiscard]] std::uint64_t obsConsumerHeartbeat() const noexcept {
    return obsConsumerHeartbeat_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::uint64_t engineConsumerHeartbeat() const noexcept {
    return engineConsumerHeartbeat_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] DiscordBridge::State discordBridgeState() const noexcept;
  [[nodiscard]] std::uint64_t discordRenderedFrames() const noexcept;

private:
  das::transport::NamedSharedMemory memory_;
  std::unique_ptr<das::transport::RingBuffer> ring_;
  das::transport::NamedSharedMemory obsMemory_;
  std::unique_ptr<das::transport::RingBuffer> obsRing_;
  das::transport::NamedSharedMemory discordMemory_;
  std::unique_ptr<das::transport::RingBuffer> discordRing_;
  std::unique_ptr<DiscordBridge> discordBridge_;
  das::audio::SampleRateConverter sampleRateConverter_;
  std::vector<float> interleaved_;
  std::array<std::vector<float>, 2> inputScratch_;
  std::atomic<double> sampleRate_ {48000.0};
  std::atomic<bool> sampleRateSupported_ {true};
  std::atomic<bool> transportReady_ {};
  std::atomic<bool> bypassed_ {};
  std::atomic<std::uint64_t> obsConsumerHeartbeat_ {};
  std::atomic<std::uint64_t> engineConsumerHeartbeat_ {};
  void writeTransport(std::uint32_t frames) noexcept;
  std::intptr_t senderGuard_ {-1};
  bool primarySender_ {true};
};
