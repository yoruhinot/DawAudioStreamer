// SPDX-License-Identifier: AGPL-3.0-only
#include "PluginEditor.h"

namespace {
constexpr auto background = 0xff171b24;
constexpr auto panel = 0xff1d2330;
constexpr auto primaryText = 0xfff2f5fa;
constexpr auto secondaryText = 0xff929db0;

void drawRaisedPanel(juce::Graphics& graphics, juce::Rectangle<float> bounds,
                     const float radius) {
  graphics.setColour(juce::Colour(0xff090b10).withAlpha(0.48F));
  graphics.fillRoundedRectangle(bounds.translated(5.0F, 6.0F), radius);
  graphics.setColour(juce::Colour(0xff30394a).withAlpha(0.52F));
  graphics.fillRoundedRectangle(bounds.translated(-2.0F, -2.0F), radius);
  graphics.setColour(juce::Colour(panel));
  graphics.fillRoundedRectangle(bounds, radius);
  graphics.setColour(juce::Colours::white.withAlpha(0.045F));
  graphics.drawRoundedRectangle(bounds.reduced(0.5F), radius, 1.0F);
}

void drawWaveIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds,
                  const juce::Colour colour) {
  const auto centre = bounds.getCentre();
  graphics.setColour(colour.withAlpha(0.18F));
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
  graphics.setColour(colour.withAlpha(0.18F));
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

  title_.setText("DAS Send", juce::dontSendNotification);
  title_.setFont(juce::FontOptions(25.0F, juce::Font::bold));
  title_.setColour(juce::Label::textColourId, juce::Colour(primaryText));
  addAndMakeVisible(title_);

  subtitle_.setText("DAW AUDIO STREAM", juce::dontSendNotification);
  subtitle_.setFont(juce::FontOptions(11.0F, juce::Font::bold));
  subtitle_.setColour(juce::Label::textColourId, juce::Colour(secondaryText));
  subtitle_.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(subtitle_);

  for (auto* heading : {&obsTitle_, &discordTitle_}) {
    heading->setFont(juce::FontOptions(13.0F, juce::Font::bold));
    heading->setColour(juce::Label::textColourId, juce::Colour(secondaryText));
    addAndMakeVisible(*heading);
  }
  obsTitle_.setText("OBS", juce::dontSendNotification);
  discordTitle_.setText("DISCORD", juce::dontSendNotification);

  for (auto* status : {&obsStatus_, &discordStatus_}) {
    status->setFont(juce::FontOptions(18.0F, juce::Font::bold));
    status->setColour(juce::Label::textColourId, juce::Colour(primaryText));
    addAndMakeVisible(*status);
  }

  detail_.setFont(juce::FontOptions(13.0F));
  detail_.setColour(juce::Label::textColourId, juce::Colour(secondaryText));
  detail_.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(detail_);

  setSize(620, 280);
  startTimerHz(5);
  timerCallback();
}

DasSendEditor::~DasSendEditor() { stopTimer(); }

juce::String DasSendEditor::text(const char* japanese, const char* english) const {
  return juce::String::fromUTF8(useJapanese_ ? japanese : english);
}

juce::Colour DasSendEditor::colourFor(const VisualState state) const {
  switch (state) {
    case VisualState::active: return juce::Colour(0xff65dda1);
    case VisualState::waiting: return juce::Colour(0xff8d9aaf);
    case VisualState::warning: return juce::Colour(0xffffbd69);
    case VisualState::error: return juce::Colour(0xffff7184);
  }
  return juce::Colour(secondaryText);
}

void DasSendEditor::updateCard(juce::Label& label, const juce::String& value,
                               const VisualState state) {
  label.setText(value, juce::dontSendNotification);
  label.setColour(juce::Label::textColourId, colourFor(state));
}

void DasSendEditor::paint(juce::Graphics& graphics) {
  auto gradient = juce::ColourGradient(juce::Colour(0xff1b202b), 0.0F, 0.0F,
                                       juce::Colour(background), 620.0F, 280.0F, false);
  graphics.setGradientFill(gradient);
  graphics.fillAll();

  const auto obsCard = juce::Rectangle<float>(24.0F, 82.0F, 278.0F, 116.0F);
  const auto discordCard = juce::Rectangle<float>(318.0F, 82.0F, 278.0F, 116.0F);
  drawRaisedPanel(graphics, obsCard, 17.0F);
  drawRaisedPanel(graphics, discordCard, 17.0F);

  drawWaveIcon(graphics, juce::Rectangle<float>(44.0F, 112.0F, 48.0F, 48.0F),
               colourFor(obsState_));
  drawShareIcon(graphics, juce::Rectangle<float>(338.0F, 112.0F, 48.0F, 48.0F),
                colourFor(discordState_));

  graphics.setColour(juce::Colour(0xff080a0f).withAlpha(0.42F));
  graphics.fillRoundedRectangle(24.0F, 220.0F, 572.0F, 38.0F, 12.0F);
  graphics.setColour(juce::Colours::white.withAlpha(0.035F));
  graphics.drawRoundedRectangle(24.5F, 220.5F, 571.0F, 37.0F, 12.0F, 1.0F);
}

void DasSendEditor::resized() {
  title_.setBounds(26, 19, 250, 40);
  subtitle_.setBounds(360, 25, 234, 26);

  obsTitle_.setBounds(108, 101, 168, 24);
  obsStatus_.setBounds(108, 126, 168, 38);
  discordTitle_.setBounds(402, 101, 168, 24);
  discordStatus_.setBounds(402, 126, 168, 38);
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
