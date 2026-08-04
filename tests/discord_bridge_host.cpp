// SPDX-License-Identifier: AGPL-3.0-only
#include "DiscordBridge.h"

#include <das/transport/NamedSharedMemory.h>
#include <das/transport/RingBuffer.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <numbers>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace {
juce::String japanese(const char* utf8) { return juce::String::fromUTF8(utf8); }

class BridgeComponent final : public juce::Component, private juce::Timer {
public:
  BridgeComponent() {
    const auto bytes = das::transport::requiredBytes(das::transport::kAudioCapacityFrames,
                                                      das::transport::kAudioChannels);
    auto mappingName = std::wstring(das::transport::kDiscordAudioMappingName) + L".manual";
#if defined(_WIN32)
    mappingName += std::to_wstring(GetCurrentProcessId());
#endif
    memory_ = das::transport::NamedSharedMemory::create(mappingName, bytes);
    if (memory_.isOpen()) {
      das::transport::RingBuffer::initialize(memory_.storage(),
                                             das::transport::kAudioCapacityFrames,
                                             das::transport::kAudioChannels,
                                             das::transport::kAudioSampleRate);
      ring_ = std::make_unique<das::transport::RingBuffer>(memory_.storage());
      if (!ring_->isCompatible()) ring_.reset();
    }
    if (ring_) {
      bridge_ = std::make_unique<DiscordBridge>(*ring_);
      bridge_->start();
      producer_ = std::thread([this] { produce(); });
    }

    title_.setText("DAS Discord Bridge Test", juce::dontSendNotification);
    title_.setFont(juce::FontOptions(24.0F, juce::Font::bold));
    title_.setColour(juce::Label::textColourId, juce::Colour(0xfff2f6ff));
    addAndMakeVisible(title_);
    status_.setFont(juce::FontOptions(18.0F, juce::Font::bold));
    addAndMakeVisible(status_);
    guide_.setText(japanese("このウィンドウをDiscordで共有すると、物理出力へ鳴らさずに\n"
                            "440 Hzの検証音を画面共有音声へ送ります。"),
                   juce::dontSendNotification);
    guide_.setColour(juce::Label::textColourId, juce::Colour(0xffaab6c8));
    addAndMakeVisible(guide_);
    setSize(620, 240);
    startTimerHz(5);
  }

  ~BridgeComponent() override {
    stopTimer();
    stopping_.store(true, std::memory_order_release);
    if (producer_.joinable()) producer_.join();
    if (bridge_) bridge_->stop();
  }

  void paint(juce::Graphics& graphics) override {
    graphics.fillAll(juce::Colour(0xff10151d));
    graphics.setColour(juce::Colour(0xff1b2431));
    graphics.fillRoundedRectangle(getLocalBounds().toFloat().reduced(20.0F).withTrimmedTop(55.0F),
                                  12.0F);
  }

  void resized() override {
    auto area = getLocalBounds().reduced(30);
    title_.setBounds(area.removeFromTop(45));
    area.removeFromTop(20);
    status_.setBounds(area.removeFromTop(40));
    guide_.setBounds(area);
  }

private:
  void timerCallback() override {
    if (bridge_ && bridge_->state() == DiscordBridge::State::virtualOutputRequired) {
      status_.setText(japanese("× 対応する無音仮想出力が必要です"),
                      juce::dontSendNotification);
      status_.setColour(juce::Label::textColourId, juce::Colour(0xffffb86b));
    } else if (!bridge_ || bridge_->state() != DiscordBridge::State::ready) {
      status_.setText(japanese("× Discordブリッジを準備できません"),
                      juce::dontSendNotification);
      status_.setColour(juce::Label::textColourId, juce::Colour(0xffff6b7a));
    } else {
      status_.setText(japanese("● 準備完了：このウィンドウを共有してください"),
                      juce::dontSendNotification);
      status_.setColour(juce::Label::textColourId, juce::Colour(0xff7ee2a8));
    }
  }

  void produce() noexcept {
    std::array<float, 480 * 2> audio {};
    double phase {};
    auto next = std::chrono::steady_clock::now();
    while (!stopping_.load(std::memory_order_acquire)) {
      for (std::size_t frame = 0; frame < 480; ++frame) {
        const auto sample = static_cast<float>(0.15 * std::sin(phase));
        phase += 2.0 * std::numbers::pi * 440.0 / 48000.0;
        audio[frame * 2] = sample;
        audio[frame * 2 + 1] = sample;
      }
      ring_->notifyProducer();
      static_cast<void>(ring_->write(audio, 480));
      next += std::chrono::milliseconds(10);
      std::this_thread::sleep_until(next);
    }
  }

  das::transport::NamedSharedMemory memory_;
  std::unique_ptr<das::transport::RingBuffer> ring_;
  std::unique_ptr<DiscordBridge> bridge_;
  std::atomic<bool> stopping_ {};
  std::thread producer_;
  juce::Label title_;
  juce::Label status_;
  juce::Label guide_;
};

class BridgeWindow final : public juce::DocumentWindow {
public:
  BridgeWindow()
      : DocumentWindow("DAS Discord Bridge Test", juce::Colour(0xff10151d),
                       DocumentWindow::closeButton) {
    setUsingNativeTitleBar(true);
    setContentOwned(new BridgeComponent(), true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
  }
  void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};

class BridgeApplication final : public juce::JUCEApplication {
public:
  const juce::String getApplicationName() override { return "DAS Discord Bridge Test"; }
  const juce::String getApplicationVersion() override { return "0.4.0"; }
  void initialise(const juce::String&) override { window_ = std::make_unique<BridgeWindow>(); }
  void shutdown() override { window_.reset(); }

private:
  std::unique_ptr<BridgeWindow> window_;
};
} // namespace

START_JUCE_APPLICATION(BridgeApplication)
