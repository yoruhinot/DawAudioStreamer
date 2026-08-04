// SPDX-License-Identifier: AGPL-3.0-only
#include "MainComponent.h"

#include <algorithm>
#include <cmath>

namespace {
juce::String japanese(const char* utf8) { return juce::String::fromUTF8(utf8); }
}

MainComponent::MainComponent() : levelBar_(displayedLevel_) {
  const auto bytes = das::transport::requiredBytes(das::transport::kAudioCapacityFrames,
                                                    das::transport::kAudioChannels);
  memory_ = das::transport::NamedSharedMemory::create(
      das::transport::kDefaultAudioMappingName, bytes);
  if (memory_.isOpen()) {
    if (!memory_.alreadyExisted())
      das::transport::RingBuffer::initialize(memory_.storage(),
                                             das::transport::kAudioCapacityFrames,
                                             das::transport::kAudioChannels,
                                             das::transport::kAudioSampleRate);
    ring_ = std::make_unique<das::transport::RingBuffer>(memory_.storage());
    if (!ring_->isCompatible()) ring_.reset();
    else ring_->discardAll();
  }

  title_.setText("DAS Engine", juce::dontSendNotification);
  title_.setFont(juce::FontOptions(28.0F, juce::Font::bold));
  title_.setColour(juce::Label::textColourId, juce::Colour(0xfff2f6ff));
  addAndMakeVisible(title_);

  connectionStatus_.setColour(juce::Label::textColourId, juce::Colour(0xff7ee2a8));
  addAndMakeVisible(connectionStatus_);
  deviceStatus_.setColour(juce::Label::textColourId, juce::Colour(0xffaab6c8));
  addAndMakeVisible(deviceStatus_);

  gainLabel_.setText(japanese("モニター音量（OBSの音量には影響しません）"), juce::dontSendNotification);
  gainLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffd7dfeb));
  addAndMakeVisible(gainLabel_);
  gainSlider_.setRange(-60.0, 6.0, 0.1);
  gainSlider_.setValue(0.0);
  gainSlider_.setTextValueSuffix(" dB");
  gainSlider_.onValueChange = [this] {
    outputGain_.setTarget(juce::Decibels::decibelsToGain(static_cast<float>(gainSlider_.getValue())));
  };
  addAndMakeVisible(gainSlider_);

  settingsButton_.setButtonText(japanese("出力デバイス設定"));
  obsButton_.setButtonText(japanese("OBSで使う手順"));
  settingsButton_.onClick = [this] { showAudioSettings(); };
  obsButton_.onClick = [this] {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon, japanese("OBSでDAS Engineを取り込む"),
        japanese("1. OBSのソース欄で［＋］を押します。\n"
        "2. ［DAS Audio（DAW）］を追加します。\n"
        "3. DAWのマスターバスに［DAS Send］を挿入します。\n\n"
        "DAWは今までどおりASIOのままで構いません。\n"
        "メーターが動けば配信へDAW音声が届いています。"));
  };
  addAndMakeVisible(settingsButton_);
  addAndMakeVisible(obsButton_);

  obsGuide_.setText(japanese("OBSには「DAS Audio（DAW）」を追加します。Engineはモニター用です。"),
                    juce::dontSendNotification);
  obsGuide_.setColour(juce::Label::textColourId, juce::Colour(0xffaab6c8));
  obsGuide_.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(obsGuide_);
  addAndMakeVisible(levelBar_);

  outputGain_.prepare(48000.0, 0.02);
  outputGain_.reset(1.0F);
  setSize(620, 390);
  setAudioChannels(0, 2);
  startTimerHz(15);
}

MainComponent::~MainComponent() {
  stopTimer();
  shutdownAudio();
}

void MainComponent::prepareToPlay(const int, const double sampleRate) {
  deviceSampleRate_ = sampleRate;
  outputGain_.prepare(sampleRate, 0.02);
  if (ring_) ring_->discardAll();
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& info) {
  info.clearActiveBufferRegion();
  if (!ring_ || info.buffer == nullptr || info.numSamples <= 0) return;
  const auto frames = static_cast<std::uint32_t>(std::min<int>(
      info.numSamples, static_cast<int>(interleaved_.size() / 2)));
  const auto read = ring_->read(std::span<float>(interleaved_.data(), frames * 2), frames);
  if (read < frames) underflows_.fetch_add(frames - read, std::memory_order_relaxed);

  float blockPeak = 0.0F;
  const auto outputChannels = std::min(2, info.buffer->getNumChannels());
  for (std::uint32_t frame = 0; frame < read; ++frame) {
    const auto gain = outputGain_.next();
    for (int channel = 0; channel < outputChannels; ++channel) {
      const auto sample = interleaved_[frame * 2 + static_cast<std::uint32_t>(channel)] * gain;
      info.buffer->setSample(channel, info.startSample + static_cast<int>(frame), sample);
      blockPeak = std::max(blockPeak, std::abs(sample));
    }
  }
  peak_.store(blockPeak, std::memory_order_relaxed);
}

void MainComponent::releaseResources() {}

void MainComponent::paint(juce::Graphics& graphics) {
  graphics.fillAll(juce::Colour(0xff10151d));
  graphics.setColour(juce::Colour(0xff1b2431));
  graphics.fillRoundedRectangle(getLocalBounds().toFloat().reduced(22.0F).withTrimmedTop(76.0F), 14.0F);
}

void MainComponent::resized() {
  auto area = getLocalBounds().reduced(34);
  title_.setBounds(area.removeFromTop(46));
  connectionStatus_.setBounds(area.removeFromTop(28));
  deviceStatus_.setBounds(area.removeFromTop(25));
  area.removeFromTop(24);
  gainLabel_.setBounds(area.removeFromTop(24));
  gainSlider_.setBounds(area.removeFromTop(46));
  levelBar_.setBounds(area.removeFromTop(18));
  area.removeFromTop(25);
  auto buttons = area.removeFromTop(38);
  settingsButton_.setBounds(buttons.removeFromLeft(210));
  buttons.removeFromLeft(14);
  obsButton_.setBounds(buttons.removeFromLeft(180));
  obsGuide_.setBounds(area.removeFromBottom(42));
}

void MainComponent::timerCallback() {
  if (ring_) {
    ring_->notifyConsumer();
    const auto heartbeat = ring_->producerHeartbeat();
    if (heartbeat != lastProducerHeartbeat_) {
      lastProducerHeartbeat_ = heartbeat;
      lastProducerActivityMs_ = juce::Time::getMillisecondCounterHiRes();
    }
  }
  const auto senderActive = lastProducerActivityMs_ > 0.0 &&
      juce::Time::getMillisecondCounterHiRes() - lastProducerActivityMs_ < 1200.0;
  connectionStatus_.setText(
      senderActive ? japanese("● DAW音声を受信中")
                   : japanese("○ DAS Sendからの音声を待っています"),
      juce::dontSendNotification);
  if (auto* device = deviceManager.getCurrentAudioDevice())
    deviceStatus_.setText(japanese("出力: ") + device->getName() + " / " +
                              juce::String(deviceSampleRate_, 0) + " Hz",
                          juce::dontSendNotification);
  else
    deviceStatus_.setText(japanese("出力デバイスがありません"), juce::dontSendNotification);
  const auto current = static_cast<double>(peak_.exchange(0.0F));
  displayedLevel_ = std::clamp(std::max(current, displayedLevel_ * 0.75), 0.0, 1.0);
}

void MainComponent::showAudioSettings() {
  auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(
      deviceManager, 0, 0, 0, 2, false, false, true, false);
  selector->setSize(520, 360);
  juce::CallOutBox::launchAsynchronously(std::move(selector), settingsButton_.getScreenBounds(), nullptr);
}
