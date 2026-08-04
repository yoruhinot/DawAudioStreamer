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
  void timerCallback() override;

  DasSendProcessor& processor_;
  juce::Label title_;
  juce::Label obsStatus_;
  juce::Label discordStatus_;
  juce::Label detail_;
  std::uint64_t lastObsHeartbeat_ {};
  double lastObsActivityMs_ {};
};
