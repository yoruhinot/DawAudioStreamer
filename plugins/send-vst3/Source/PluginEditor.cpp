// SPDX-License-Identifier: AGPL-3.0-only
#include "PluginEditor.h"

namespace {
juce::String japanese(const char* utf8) { return juce::String::fromUTF8(utf8); }
}

DasSendEditor::DasSendEditor(DasSendProcessor& processor)
    : AudioProcessorEditor(processor), processor_(processor) {
  title_.setText("DAS Send", juce::dontSendNotification);
  title_.setFont(juce::FontOptions(26.0F, juce::Font::bold));
  title_.setColour(juce::Label::textColourId, juce::Colour(0xfff2f6ff));
  addAndMakeVisible(title_);

  for (auto* label : {&obsStatus_, &discordStatus_}) {
    label->setFont(juce::FontOptions(17.0F, juce::Font::bold));
    addAndMakeVisible(*label);
  }
  detail_.setColour(juce::Label::textColourId, juce::Colour(0xffaab6c8));
  detail_.setJustificationType(juce::Justification::topLeft);
  addAndMakeVisible(detail_);

  setSize(650, 250);
  startTimerHz(5);
  timerCallback();
}

DasSendEditor::~DasSendEditor() { stopTimer(); }

void DasSendEditor::paint(juce::Graphics& graphics) {
  graphics.fillAll(juce::Colour(0xff10151d));
  graphics.setColour(juce::Colour(0xff1b2431));
  graphics.fillRoundedRectangle(
      getLocalBounds().toFloat().reduced(18.0F).withTrimmedTop(52.0F), 12.0F);
}

void DasSendEditor::resized() {
  auto area = getLocalBounds().reduced(28);
  title_.setBounds(area.removeFromTop(44));
  area.removeFromTop(8);
  obsStatus_.setBounds(area.removeFromTop(36));
  discordStatus_.setBounds(area.removeFromTop(36));
  area.removeFromTop(8);
  detail_.setBounds(area);
}

void DasSendEditor::timerCallback() {
  const auto now = juce::Time::getMillisecondCounterHiRes();
  const auto recordActivity = [now](const std::uint64_t heartbeat,
                                    std::uint64_t& previous,
                                    double& activityMs) {
    if (heartbeat != previous) {
      previous = heartbeat;
      activityMs = now;
    }
  };
  recordActivity(processor_.obsConsumerHeartbeat(), lastObsHeartbeat_, lastObsActivityMs_);

  const auto isActive = [now](const double activityMs) {
    return activityMs > 0.0 && now - activityMs < 1500.0;
  };
  const auto obsActive = isActive(lastObsActivityMs_);
  const auto green = juce::Colour(0xff7ee2a8);
  const auto waiting = juce::Colour(0xffaab6c8);
  const auto warning = juce::Colour(0xffffb86b);
  const auto error = juce::Colour(0xffff6b7a);

  if (!processor_.isPrimarySender()) {
    obsStatus_.setText(japanese("OBS  △ DAS Sendが複数あります"), juce::dontSendNotification);
    discordStatus_.setText(japanese("Discord  △ このDAS Sendは待機中です"),
                           juce::dontSendNotification);
    obsStatus_.setColour(juce::Label::textColourId, warning);
    discordStatus_.setColour(juce::Label::textColourId, warning);
    detail_.setText(japanese("音を二重送信しないよう、このインスタンスは待機しています。\n"
                             "マスターバスのDAS Sendを1個だけ残してください。"),
                    juce::dontSendNotification);
  } else if (!processor_.sampleRateSupported()) {
    obsStatus_.setText(japanese("OBS  × サンプルレートに対応できません"),
                       juce::dontSendNotification);
    discordStatus_.setText(japanese("Discord  × サンプルレートに対応できません"),
                           juce::dontSendNotification);
    obsStatus_.setColour(juce::Label::textColourId, error);
    discordStatus_.setColour(juce::Label::textColourId, error);
    detail_.setText(japanese("現在: ") + juce::String(processor_.currentSampleRate(), 0) +
                        japanese(" Hz\nDAWのサンプルレートを8～384 kHzにしてください。"),
                    juce::dontSendNotification);
  } else if (!processor_.transportReady()) {
    obsStatus_.setText(japanese("OBS  × 音声送信を開始できません"),
                       juce::dontSendNotification);
    discordStatus_.setText(japanese("Discord  × 音声送信を開始できません"),
                           juce::dontSendNotification);
    obsStatus_.setColour(juce::Label::textColourId, error);
    discordStatus_.setColour(juce::Label::textColourId, error);
    detail_.setText(japanese("OBSとDAWを終了して、OBS→DAWの順に起動し直してください。"),
                    juce::dontSendNotification);
  } else {
    obsStatus_.setText(obsActive ? japanese("OBS  ● DAS Audioへ送信中")
                                 : japanese("OBS  ○ DAS Audioを待っています"),
                       juce::dontSendNotification);
    obsStatus_.setColour(juce::Label::textColourId, obsActive ? green : waiting);

    #if defined(__APPLE__)
    discordStatus_.setText(japanese("Discord  ● macOSの共有音声を使用できます"),
                           juce::dontSendNotification);
    discordStatus_.setColour(juce::Label::textColourId, green);
    detail_.setText(japanese(
        "DiscordではDAWアプリまたは画面全体を共有すると、macOSが音声を載せます。\n"
        "OBSでは音声ソース「DAS Audio（DAW）」を1つ追加してください。"),
        juce::dontSendNotification);
    #else
    const auto bridgeState = processor_.discordBridgeState();
    if (bridgeState == DiscordBridge::State::ready) {
      discordStatus_.setText(japanese("Discord  ● 直接共有用の音声を準備済み"),
                             juce::dontSendNotification);
      discordStatus_.setColour(juce::Label::textColourId, green);
    } else if (bridgeState == DiscordBridge::State::virtualOutputRequired) {
      discordStatus_.setText(japanese("Discord  × VB-CABLEを追加してください"),
                             juce::dontSendNotification);
      discordStatus_.setColour(juce::Label::textColourId, warning);
    } else if (bridgeState == DiscordBridge::State::starting) {
      discordStatus_.setText(japanese("Discord  ○ 音声を準備中"),
                             juce::dontSendNotification);
      discordStatus_.setColour(juce::Label::textColourId, waiting);
    } else {
      discordStatus_.setText(japanese("Discord  × 音声を準備できません"),
                             juce::dontSendNotification);
      discordStatus_.setColour(juce::Label::textColourId, error);
    }

    detail_.setText(japanese(
        "DiscordではDAWアプリまたは画面全体をそのまま共有します。マイク設定は変更しません。\n"
        "OBSでは音声ソース「DAS Audio（DAW）」を1つ追加してください。"),
        juce::dontSendNotification);
    #endif
  }
}
