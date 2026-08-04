// SPDX-License-Identifier: AGPL-3.0-only
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace {
class EditorWindow final : public juce::DocumentWindow {
public:
  explicit EditorWindow(juce::AudioPluginInstance& plugin)
      : DocumentWindow("DAS Send UI Test", juce::Colour(0xff10151d),
                       DocumentWindow::closeButton) {
    setUsingNativeTitleBar(true);
    setContentOwned(plugin.createEditor(), true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
  }

  void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};

class EditorHostApplication final : public juce::JUCEApplication {
public:
  const juce::String getApplicationName() override { return "DAS Send UI Test"; }
  const juce::String getApplicationVersion() override { return "0.4.0"; }

  void initialise(const juce::String&) override {
#if defined(_WIN32)
    const auto suffix = std::wstring(L"editor-host-") + std::to_wstring(GetCurrentProcessId());
    SetEnvironmentVariableW(L"DAS_TEST_NAMESPACE", suffix.c_str());
#endif
    auto buildRoot = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                         .getParentDirectory()
                         .getParentDirectory()
                         .getParentDirectory()
                         .getParentDirectory();
    const auto bundle = buildRoot.getChildFile("plugins/send-vst3/DasSend_artefacts/Release/VST3/DAS Send.vst3");
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format_.findAllTypesForFile(descriptions, bundle.getFullPathName());
    if (descriptions.size() != 1) {
      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                             "DAS Send UI Test", "VST3を読み込めませんでした");
      return;
    }
    juce::String error;
    plugin_ = format_.createInstanceFromDescription(*descriptions[0], 48000.0, 512, error);
    if (!plugin_) {
      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                             "DAS Send UI Test", error);
      return;
    }
    plugin_->setPlayConfigDetails(2, 2, 48000.0, 512);
    plugin_->prepareToPlay(48000.0, 512);
    window_ = std::make_unique<EditorWindow>(*plugin_);
  }

  void shutdown() override {
    window_.reset();
    if (plugin_) plugin_->releaseResources();
    plugin_.reset();
  }

private:
  juce::VST3PluginFormat format_;
  std::unique_ptr<juce::AudioPluginInstance> plugin_;
  std::unique_ptr<EditorWindow> window_;
};
} // namespace

START_JUCE_APPLICATION(EditorHostApplication)
