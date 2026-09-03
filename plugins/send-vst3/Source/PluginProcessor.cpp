// SPDX-License-Identifier: AGPL-3.0-only
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#elif defined(__APPLE__)
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
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
#else
  if (const auto* suffix = std::getenv("DAS_TEST_NAMESPACE");
      suffix != nullptr && *suffix != '\0') {
    name.push_back(L'.');
    while (*suffix != '\0') name.push_back(static_cast<unsigned char>(*suffix++));
  }
#endif
  return name;
}

#if defined(__APPLE__)
std::string senderLockPath() {
  std::string path = "/tmp/org.dawaudiostreamer.send.owner." +
                     std::to_string(static_cast<unsigned long long>(getuid()));
  if (const auto* suffix = std::getenv("DAS_TEST_NAMESPACE");
      suffix != nullptr && *suffix != '\0') {
    path.push_back('.');
    while (*suffix != '\0') {
      const auto value = static_cast<unsigned char>(*suffix++);
      const bool safe = (value >= 'a' && value <= 'z') ||
                        (value >= 'A' && value <= 'Z') ||
                        (value >= '0' && value <= '9') || value == '-' || value == '_';
      path.push_back(safe ? static_cast<char>(value) : '_');
    }
  }
  return path;
}
#endif

std::unique_ptr<das::transport::RingBuffer> prepareSharedRing(
    das::transport::NamedSharedMemory& memory, const std::wstring_view name,
    const std::size_t bytes) {
  memory = das::transport::NamedSharedMemory::create(namespacedName(name), bytes);
  if (!memory.isOpen()) return {};
  auto ring = std::make_unique<das::transport::RingBuffer>(memory.storage());
  if (!ring->isCompatible()) {
    if (!das::transport::RingBuffer::initialize(memory.storage(),
                                                das::transport::kAudioCapacityFrames,
                                                das::transport::kAudioChannels,
                                                das::transport::kAudioSampleRate))
      return {};
    ring = std::make_unique<das::transport::RingBuffer>(memory.storage());
  }
  if (!ring->isCompatible()) return {};
  ring->discardAll();
  return ring;
}
}

DasSendProcessor::DasSendProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
#if defined(_WIN32)
  const auto guardName = namespacedName(L"Local\\DawAudioStreamer.Send.Owner.v1");
  const auto guard = CreateEventW(nullptr, TRUE, FALSE, guardName.c_str());
  if (guard != nullptr) senderGuard_ = reinterpret_cast<std::intptr_t>(guard);
  if (guard != nullptr && GetLastError() == ERROR_ALREADY_EXISTS) {
    primarySender_ = false;
    CloseHandle(guard);
    senderGuard_ = -1;
  }
#elif defined(__APPLE__)
  const auto lockPath = senderLockPath();
  const auto guard = ::open(lockPath.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (guard < 0 || flock(guard, LOCK_EX | LOCK_NB) != 0) {
    primarySender_ = false;
    if (guard >= 0) ::close(guard);
  } else {
    senderGuard_ = guard;
  }
#endif
}

DasSendProcessor::~DasSendProcessor() {
  releaseResources();
#if defined(_WIN32)
  if (senderGuard_ != -1)
    CloseHandle(reinterpret_cast<HANDLE>(senderGuard_));
#elif defined(__APPLE__)
  if (senderGuard_ != -1) {
    static_cast<void>(flock(static_cast<int>(senderGuard_), LOCK_UN));
    ::close(static_cast<int>(senderGuard_));
  }
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
  ring_ = prepareSharedRing(memory_, das::transport::kDefaultAudioMappingName, bytes);
  obsRing_ = prepareSharedRing(obsMemory_, das::transport::kObsAudioMappingName, bytes);
#if defined(_WIN32)
  discordRing_ = prepareSharedRing(discordMemory_,
                                   das::transport::kDiscordAudioMappingName, bytes);
  if (discordRing_) {
    discordBridge_ = std::make_unique<DiscordBridge>(*discordRing_);
    discordBridge_->start();
  }
#endif
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
