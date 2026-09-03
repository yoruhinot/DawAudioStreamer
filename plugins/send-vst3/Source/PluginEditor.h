// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

class DasSendEditor final : public juce::AudioProcessorEditor,
                            private juce::Timer {
public:
  explicit DasSendEditor(DasSendProcessor& processor);
  ~DasSendEditor() override;
  void paint(juce::Graphics& graphics) override;
  void resized() override;

private:
  enum class VisualState { active, waiting, warning, error };

  void timerCallback() override;
  void updateCard(juce::Label& label, const juce::String& text, VisualState state);
  [[nodiscard]] juce::Colour colourFor(VisualState state) const;
  [[nodiscard]] juce::String text(const char* japanese, const char* english) const;

  DasSendProcessor& processor_;
  bool useJapanese_ {};
  VisualState obsState_ {VisualState::waiting};
  VisualState discordState_ {VisualState::waiting};
  juce::Label title_;
  juce::Label subtitle_;
  juce::Label obsTitle_;
  juce::Label obsStatus_;
  juce::Label discordTitle_;
  juce::Label discordStatus_;
  juce::Label detail_;
  std::uint64_t lastObsHeartbeat_ {};
  double lastObsActivityMs_ {};
};
