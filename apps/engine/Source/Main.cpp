// SPDX-License-Identifier: AGPL-3.0-only
#include "MainComponent.h"

#include <juce_gui_extra/juce_gui_extra.h>

class DasEngineApplication final : public juce::JUCEApplication {
public:
  const juce::String getApplicationName() override { return "DAS Engine"; }
  const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
  bool moreThanOneInstanceAllowed() override { return false; }

  void initialise(const juce::String&) override {
    const auto translations = juce::String::fromUTF8(
        "language: Japanese\n"
        "countries: jp\n"
        "\"Audio device type:\" = \"オーディオ方式:\"\n"
        "\"Output:\" = \"出力:\"\n"
        "\"Active output channels:\" = \"使用する出力チャンネル:\"\n"
        "\"Sample rate:\" = \"サンプルレート:\"\n"
        "\"Audio buffer size:\" = \"バッファサイズ:\"\n"
        "\"Test\" = \"テスト\"\n"
        "\"Plays a test tone\" = \"テスト音を再生します\"\n");
    juce::LocalisedStrings::setCurrentMappings(new juce::LocalisedStrings(translations, false));
    mainWindow_ = std::make_unique<MainWindow>(getApplicationName());
  }

  void shutdown() override {
    mainWindow_.reset();
    juce::LocalisedStrings::setCurrentMappings(nullptr);
  }

  void systemRequestedQuit() override { quit(); }
  void anotherInstanceStarted(const juce::String&) override {}

private:
  class MainWindow final : public juce::DocumentWindow {
  public:
    explicit MainWindow(const juce::String& name)
        : DocumentWindow(name, juce::Colour(0xff10151d), allButtons) {
      setUsingNativeTitleBar(true);
      setContentOwned(new MainComponent(), true);
      setResizable(true, true);
      centreWithSize(getWidth(), getHeight());
      setVisible(true);
    }

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
  };

  std::unique_ptr<MainWindow> mainWindow_;
};

START_JUCE_APPLICATION(DasEngineApplication)
