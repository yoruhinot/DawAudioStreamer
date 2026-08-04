// SPDX-License-Identifier: AGPL-3.0-only
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cstring>
#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace {
std::wstring namespacedName(const std::wstring_view base) {
  std::wstring name(base);
#if defined(_WIN32)
  wchar_t suffix[128] {};
  const auto length = GetEnvironmentVariableW(L"DAS_TEST_NAMESPACE", suffix,
                                               static_cast<DWORD>(std::size(suffix)));
  if (length > 0 && length < std::size(suffix)) {
    name.push_back(L'.');
    name.append(suffix, length);
  }
#endif
  return name;
}
}

DasSendProcessor::DasSendProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
#if defined(_WIN32)
  const auto guardName = namespacedName(L"Local\\DawAudioStreamer.Send.Owner.v1");
  const auto guard = CreateEventW(nullptr, TRUE, FALSE, guardName.c_str());
  senderGuard_ = guard;
  if (guard != nullptr && GetLastError() == ERROR_ALREADY_EXISTS) {
    primarySender_ = false;
    CloseHandle(guard);
    senderGuard_ = nullptr;
  }
#endif
}

DasSendProcessor::~DasSendProcessor() {
  releaseResources();
#if defined(_WIN32)
  if (senderGuard_ != nullptr) CloseHandle(static_cast<HANDLE>(senderGuard_));
#endif
}

void DasSendProcessor::prepareToPlay(const double sampleRate, const int samplesPerBlock) {
  releaseResources();
  // r8brain's internal latency scales with its maximum block size. DAWs may still
  // deliver a larger block occasionally, so processBlock chunks it safely.
  const auto maximumInputFrames = static_cast<std::size_t>(std::max(1, samplesPerBlock));
  const auto converterReady = sampleRateConverter_.prepare(
      sampleRate, static_cast<double>(das::transport::kAudioSampleRate), maximumInputFrames);
  for (auto& input : inputScratch_) input.resize(maximumInputFrames);
  interleaved_.resize(sampleRateConverter_.maximumOutputFrames() * 2);
  sampleRate_.store(sampleRate);
  sampleRateSupported_.store(converterReady && sampleRate >= 8000.0 && sampleRate <= 384000.0);
  if (!primarySender_) {
    transportReady_.store(false);
    return;
  }
  const auto bytes = das::transport::requiredBytes(das::transport::kAudioCapacityFrames,
                                                    das::transport::kAudioChannels);
  memory_ = das::transport::NamedSharedMemory::create(
      namespacedName(das::transport::kDefaultAudioMappingName), bytes);
  if (memory_.isOpen()) {
    if (!memory_.alreadyExisted())
      das::transport::RingBuffer::initialize(memory_.storage(),
                                             das::transport::kAudioCapacityFrames,
                                             das::transport::kAudioChannels,
                                             das::transport::kAudioSampleRate);
    ring_ = std::make_unique<das::transport::RingBuffer>(memory_.storage());
    if (!ring_->isCompatible()) ring_.reset();
  }
  obsMemory_ = das::transport::NamedSharedMemory::create(
      namespacedName(das::transport::kObsAudioMappingName), bytes);
  if (obsMemory_.isOpen()) {
    if (!obsMemory_.alreadyExisted())
      das::transport::RingBuffer::initialize(obsMemory_.storage(),
                                             das::transport::kAudioCapacityFrames,
                                             das::transport::kAudioChannels,
                                             das::transport::kAudioSampleRate);
    obsRing_ = std::make_unique<das::transport::RingBuffer>(obsMemory_.storage());
    if (!obsRing_->isCompatible()) obsRing_.reset();
  }
  discordMemory_ = das::transport::NamedSharedMemory::create(
      namespacedName(das::transport::kDiscordAudioMappingName), bytes);
  if (discordMemory_.isOpen()) {
    if (!discordMemory_.alreadyExisted())
      das::transport::RingBuffer::initialize(discordMemory_.storage(),
                                             das::transport::kAudioCapacityFrames,
                                             das::transport::kAudioChannels,
                                             das::transport::kAudioSampleRate);
    discordRing_ = std::make_unique<das::transport::RingBuffer>(discordMemory_.storage());
    if (!discordRing_->isCompatible()) discordRing_.reset();
  }
  if (discordRing_) {
    discordBridge_ = std::make_unique<DiscordBridge>(*discordRing_);
    discordBridge_->start();
  }
  transportReady_.store(ring_ != nullptr || obsRing_ != nullptr || discordRing_ != nullptr);
}

void DasSendProcessor::releaseResources() {
  if (discordBridge_) discordBridge_->stop();
  discordBridge_.reset();
  discordRing_.reset();
  discordMemory_ = {};
  ring_.reset();
  memory_ = {};
  obsRing_.reset();
  obsMemory_ = {};
  transportReady_.store(false);
}

bool DasSendProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto input = layouts.getMainInputChannelSet();
  return input == layouts.getMainOutputChannelSet() &&
         (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

void DasSendProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  juce::ScopedNoDenormals noDenormals;
  if (!primarySender_ || (!ring_ && !obsRing_ && !discordRing_) ||
      !sampleRateSupported_.load(std::memory_order_relaxed)) return;
  const auto inputFrames = static_cast<std::size_t>(buffer.getNumSamples());
  const auto channels = buffer.getNumChannels();
  std::size_t offset {};
  while (offset < inputFrames) {
    const auto chunk = std::min(inputFrames - offset, sampleRateConverter_.maximumInputFrames());
    for (std::size_t frame = 0; frame < chunk; ++frame) {
      inputScratch_[0][frame] = buffer.getSample(0, static_cast<int>(offset + frame));
      inputScratch_[1][frame] = buffer.getSample(
          std::min(1, channels - 1), static_cast<int>(offset + frame));
    }
    const auto frames = sampleRateConverter_.process(
        std::span<const float>(inputScratch_[0].data(), chunk),
        std::span<const float>(inputScratch_[1].data(), chunk), interleaved_);
    writeTransport(frames);
    offset += chunk;
  }
}

void DasSendProcessor::writeTransport(const std::uint32_t frames) noexcept {
  if (frames == 0) return;
  const auto samples = std::span<const float>(interleaved_.data(), frames * 2);
  if (ring_) {
    ring_->notifyProducer();
    engineConsumerHeartbeat_.store(ring_->consumerHeartbeat(), std::memory_order_relaxed);
    static_cast<void>(ring_->write(samples, frames));
  }
  if (obsRing_) {
    obsRing_->notifyProducer();
    obsConsumerHeartbeat_.store(obsRing_->consumerHeartbeat(), std::memory_order_relaxed);
    static_cast<void>(obsRing_->write(samples, frames));
  }
  if (discordRing_) {
    discordRing_->notifyProducer();
    static_cast<void>(discordRing_->write(samples, frames));
  }
}

DiscordBridge::State DasSendProcessor::discordBridgeState() const noexcept {
  return discordBridge_ ? discordBridge_->state() : DiscordBridge::State::stopped;
}

std::uint64_t DasSendProcessor::discordRenderedFrames() const noexcept {
  return discordBridge_ ? discordBridge_->renderedFrames() : 0;
}

juce::AudioProcessorEditor* DasSendProcessor::createEditor() {
  return new DasSendEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DasSendProcessor(); }
