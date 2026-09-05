// SPDX-License-Identifier: AGPL-3.0-only
#include "PluginEditor.h"

namespace {
struct Theme {
  juce::Colour parchment;
  juce::Colour card;
  juce::Colour ink;
  juce::Colour graphite;
  juce::Colour linen;
  juce::Colour smoke;
  juce::Colour markerYellow;
  juce::Colour detail;
};

Theme themeFor(const bool darkMode) {
  if (darkMode) {
    return {juce::Colour(0xff151411), juce::Colour(0xff211f1b),
            juce::Colour(0xfff7f3eb), juce::Colour(0xffbcb4a5),
            juce::Colour(0xff484239), juce::Colour(0xff3a3630),
            juce::Colour(0xfff2cd32), juce::Colour(0xff2a2721)};
  }
  return {juce::Colour(0xffded5c8), juce::Colour(0xfff3eee7),
          juce::Colour(0xff000000), juce::Colour(0xff6a6559),
          juce::Colour(0xffc8beb1), juce::Colour(0xffddd5ca),
          juce::Colour(0xffffdd33), juce::Colour(0xffd2c7b8)};
}

void drawRaisedPanel(juce::Graphics& graphics, juce::Rectangle<float> bounds,
                     const float radius, const Theme& theme) {
  graphics.setColour(theme.card);
  graphics.fillRoundedRectangle(bounds, radius);
  graphics.setColour(theme.linen);
  graphics.drawRoundedRectangle(bounds.reduced(0.5F), radius, 1.0F);
}

void drawWaveIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds,
                  const juce::Colour colour) {
  const auto centre = bounds.getCentre();
  graphics.setColour(colour.withAlpha(0.11F));
  graphics.fillEllipse(bounds);
  graphics.setColour(colour);
  graphics.drawEllipse(bounds.reduced(0.5F), 1.4F);
  const float heights[] {8.0F, 16.0F, 23.0F, 14.0F, 7.0F};
  auto x = centre.x - 9.0F;
  for (const auto height : heights) {
    graphics.drawLine(x, centre.y - height * 0.5F, x, centre.y + height * 0.5F, 2.1F);
    x += 4.5F;
  }
}

void drawShareIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds,
                   const juce::Colour colour) {
  graphics.setColour(colour.withAlpha(0.11F));
  graphics.fillEllipse(bounds);
  graphics.setColour(colour);
  auto screen = bounds.reduced(9.0F, 11.0F).withTrimmedBottom(4.0F);
  graphics.drawRoundedRectangle(screen, 2.0F, 1.6F);
  const auto centreX = screen.getCentreX();
  graphics.drawLine(centreX, screen.getBottom(), centreX, screen.getBottom() + 4.0F, 1.5F);
  graphics.drawLine(centreX - 5.0F, screen.getBottom() + 4.0F,
                    centreX + 5.0F, screen.getBottom() + 4.0F, 1.5F);
  auto arrow = juce::Path();
  arrow.startNewSubPath(screen.getCentreX() - 4.0F, screen.getCentreY() + 2.0F);
  arrow.lineTo(screen.getCentreX() + 4.0F, screen.getCentreY() - 6.0F);
  arrow.lineTo(screen.getCentreX() + 4.0F, screen.getCentreY() - 1.0F);
  arrow.lineTo(screen.getCentreX() + 8.0F, screen.getCentreY() - 1.0F);
  graphics.strokePath(arrow, juce::PathStrokeType(1.7F));
}
}

DasSendEditor::DasSendEditor(DasSendProcessor& processor)
    : AudioProcessorEditor(processor), processor_(processor) {
  useJapanese_ = juce::SystemStats::getUserLanguage().startsWithIgnoreCase("ja");
  darkMode_ = juce::Desktop::getInstance().isDarkModeActive();
  const auto theme = themeFor(darkMode_);

  title_.setText("DAS Send", juce::dontSendNotification);
  title_.setFont(juce::FontOptions(25.0F, juce::Font::bold));
  title_.setColour(juce::Label::textColourId, theme.ink);
  addAndMakeVisible(title_);

  subtitle_.setText("DAW AUDIO STREAM", juce::dontSendNotification);
  subtitle_.setFont(juce::FontOptions(11.0F, juce::Font::bold));
  subtitle_.setColour(juce::Label::textColourId, theme.graphite);
  subtitle_.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(subtitle_);

  for (auto* heading : {&obsTitle_, &discordTitle_}) {
    heading->setFont(juce::FontOptions(13.0F, juce::Font::bold));
    heading->setColour(juce::Label::textColourId, theme.graphite);
    addAndMakeVisible(*heading);
  }
  obsTitle_.setText("OBS", juce::dontSendNotification);
  discordTitle_.setText("DISCORD", juce::dontSendNotification);

  for (auto* status : {&obsStatus_, &discordStatus_}) {
    status->setFont(juce::FontOptions(18.0F, juce::Font::bold));
    status->setColour(juce::Label::textColourId, theme.ink);
    addAndMakeVisible(*status);
  }

  detail_.setFont(juce::FontOptions(13.0F));
  detail_.setColour(juce::Label::textColourId, theme.graphite);
  detail_.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(detail_);

  setSize(620, 280);
  juce::Desktop::getInstance().addDarkModeSettingListener(this);
  startTimerHz(5);
  timerCallback();
}

DasSendEditor::~DasSendEditor() {
  juce::Desktop::getInstance().removeDarkModeSettingListener(this);
  stopTimer();
}

juce::String DasSendEditor::text(const char* japanese, const char* english) const {
  return juce::String::fromUTF8(useJapanese_ ? japanese : english);
}

juce::Colour DasSendEditor::colourFor(const VisualState state) const {
  const auto theme = themeFor(darkMode_);
  switch (state) {
    case VisualState::active:
      return darkMode_ ? juce::Colour(0xff95c96a) : juce::Colour(0xff3e6b15);
    case VisualState::waiting: return theme.graphite;
    case VisualState::warning:
      return darkMode_ ? theme.markerYellow : juce::Colour(0xffaa7e2e);
    case VisualState::error:
      return darkMode_ ? juce::Colour(0xffff7b59) : juce::Colour(0xffc74725);
  }
  return theme.graphite;
}

void DasSendEditor::darkModeSettingChanged() {
  darkMode_ = juce::Desktop::getInstance().isDarkModeActive();
  applyTheme();
}

void DasSendEditor::applyTheme() {
  const auto theme = themeFor(darkMode_);
  title_.setColour(juce::Label::textColourId, theme.ink);
  subtitle_.setColour(juce::Label::textColourId, theme.graphite);
  obsTitle_.setColour(juce::Label::textColourId, theme.graphite);
  discordTitle_.setColour(juce::Label::textColourId, theme.graphite);
  detail_.setColour(juce::Label::textColourId, theme.graphite);
  timerCallback();
}

void DasSendEditor::updateCard(juce::Label& label, const juce::String& value,
                               const VisualState state) {
  const auto theme = themeFor(darkMode_);
  label.setText(value, juce::dontSendNotification);
  label.setColour(juce::Label::textColourId,
                  state == VisualState::active || state == VisualState::error
                      ? juce::Colours::white
                      : state == VisualState::warning ? juce::Colours::black : theme.ink);
}

void DasSendEditor::paint(juce::Graphics& graphics) {
  const auto theme = themeFor(darkMode_);
  graphics.fillAll(theme.parchment);

  auto marker = juce::Path();
  marker.startNewSubPath(24.0F, 46.0F);
  marker.lineTo(161.0F, 42.0F);
  marker.lineTo(165.0F, 58.0F);
  marker.lineTo(22.0F, 61.0F);
  marker.closeSubPath();
  graphics.setColour(theme.markerYellow.withAlpha(0.9F));
  graphics.fillPath(marker);

  const auto obsCard = juce::Rectangle<float>(24.0F, 82.0F, 278.0F, 116.0F);
  const auto discordCard = juce::Rectangle<float>(318.0F, 82.0F, 278.0F, 116.0F);
  drawRaisedPanel(graphics, obsCard, 12.0F, theme);
  drawRaisedPanel(graphics, discordCard, 12.0F, theme);

  const auto statusFill = [&theme](const VisualState state) {
    switch (state) {
      case VisualState::active: return juce::Colour(0xff3e6b15);
      case VisualState::waiting: return theme.smoke;
      case VisualState::warning: return theme.markerYellow;
      case VisualState::error: return juce::Colour(0xffc74725);
    }
    return theme.smoke;
  };
  graphics.setColour(statusFill(obsState_));
  graphics.fillRoundedRectangle(106.0F, 130.0F, 168.0F, 32.0F, 16.0F);
  graphics.setColour(statusFill(discordState_));
  graphics.fillRoundedRectangle(400.0F, 130.0F, 168.0F, 32.0F, 16.0F);

  drawWaveIcon(graphics, juce::Rectangle<float>(44.0F, 112.0F, 48.0F, 48.0F),
               colourFor(obsState_));
  drawShareIcon(graphics, juce::Rectangle<float>(338.0F, 112.0F, 48.0F, 48.0F),
                colourFor(discordState_));

  graphics.setColour(theme.detail);
  graphics.fillRoundedRectangle(24.0F, 220.0F, 572.0F, 38.0F, 12.0F);
  graphics.setColour(theme.linen);
  graphics.drawRoundedRectangle(24.5F, 220.5F, 571.0F, 37.0F, 12.0F, 1.0F);
}

void DasSendEditor::resized() {
  title_.setBounds(26, 19, 250, 40);
  subtitle_.setBounds(360, 25, 234, 26);

  obsTitle_.setBounds(108, 101, 168, 24);
  obsStatus_.setBounds(119, 127, 144, 38);
  discordTitle_.setBounds(402, 101, 168, 24);
  discordStatus_.setBounds(413, 127, 144, 38);
  detail_.setBounds(42, 222, 536, 34);
}

void DasSendEditor::timerCallback() {
  const auto now = juce::Time::getMillisecondCounterHiRes();
  const auto heartbeat = processor_.obsConsumerHeartbeat();
  if (heartbeat != lastObsHeartbeat_) {
    lastObsHeartbeat_ = heartbeat;
    lastObsActivityMs_ = now;
  }
  const auto obsActive = lastObsActivityMs_ > 0.0 && now - lastObsActivityMs_ < 1500.0;

  if (!processor_.isPrimarySender()) {
    obsState_ = VisualState::warning;
    discordState_ = VisualState::warning;
    updateCard(obsStatus_, text("待機", "PAUSED"), obsState_);
    updateCard(discordStatus_, text("待機", "PAUSED"), discordState_);
    detail_.setText(text("DAS Sendは1個だけ使用してください。追加分は音を送りません。",
                         "Use one DAS Send only. Additional instances do not send audio."),
                    juce::dontSendNotification);
  } else if (processor_.isBypassed()) {
    obsState_ = VisualState::waiting;
    discordState_ = VisualState::waiting;
    updateCard(obsStatus_, text("バイパス中", "BYPASSED"), obsState_);
    updateCard(discordStatus_, text("バイパス中", "BYPASSED"), discordState_);
    detail_.setText(text("DAS Sendを有効にすると配信を再開します。",
                         "Enable DAS Send to resume streaming."),
                    juce::dontSendNotification);
  } else if (!processor_.sampleRateSupported()) {
    obsState_ = VisualState::error;
    discordState_ = VisualState::error;
    updateCard(obsStatus_, text("非対応", "UNSUPPORTED"), obsState_);
    updateCard(discordStatus_, text("非対応", "UNSUPPORTED"), discordState_);
    detail_.setText(text("DAWのサンプルレートを8～384 kHzに設定してください。",
                         "Set the DAW sample rate between 8 and 384 kHz."),
                    juce::dontSendNotification);
  } else if (!processor_.transportReady()) {
    obsState_ = VisualState::error;
    discordState_ = VisualState::error;
    updateCard(obsStatus_, text("エラー", "ERROR"), obsState_);
    updateCard(discordStatus_, text("エラー", "ERROR"), discordState_);
    detail_.setText(text("OBSとDAWを終了し、OBS→DAWの順に起動し直してください。",
                         "Quit both apps, then restart OBS before the DAW."),
                    juce::dontSendNotification);
  } else {
    obsState_ = obsActive ? VisualState::active : VisualState::waiting;
    updateCard(obsStatus_, obsActive ? text("送信中", "STREAMING")
                                     : text("接続待ち", "WAITING"),
               obsState_);

#if defined(__APPLE__)
    discordState_ = VisualState::active;
    updateCard(discordStatus_, text("OS音声共有", "SYSTEM AUDIO"), discordState_);
    detail_.setText(text("OBS：DAS Audioを追加　／　Discord：macOSの画面共有を使用",
                         "OBS: add DAS Audio  /  Discord: use macOS screen sharing"),
                    juce::dontSendNotification);
#else
    const auto bridgeState = processor_.discordBridgeState();
    if (bridgeState == DiscordBridge::State::ready) {
      discordState_ = VisualState::active;
      updateCard(discordStatus_, text("準備完了", "READY"), discordState_);
    } else if (bridgeState == DiscordBridge::State::virtualOutputRequired) {
      discordState_ = VisualState::warning;
      updateCard(discordStatus_, "VB-CABLE", discordState_);
    } else if (bridgeState == DiscordBridge::State::starting) {
      discordState_ = VisualState::waiting;
      updateCard(discordStatus_, text("準備中", "STARTING"), discordState_);
    } else {
      discordState_ = VisualState::error;
      updateCard(discordStatus_, text("エラー", "ERROR"), discordState_);
    }

    if (bridgeState == DiscordBridge::State::virtualOutputRequired) {
      detail_.setText(text("DiscordにはVB-CABLEが必要です。OBSはそのまま使えます。",
                           "Discord needs VB-CABLE. OBS is ready to use."),
                      juce::dontSendNotification);
    } else {
      detail_.setText(text("OBS：DAS Audioを追加　／　Discord：DAWまたは画面全体を共有",
                           "OBS: add DAS Audio  /  Discord: share the DAW or your screen"),
                      juce::dontSendNotification);
    }
#endif
  }

  repaint();
}
