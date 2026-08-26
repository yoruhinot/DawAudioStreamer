// SPDX-License-Identifier: AGPL-3.0-only
#include <das/transport/NamedSharedMemory.h>
#include <das/transport/RingBuffer.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <iostream>
#include <memory>
#include <numbers>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Audioclient.h>
#include <propkeydef.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <Mmdeviceapi.h>
#include <Propsys.h>
#include <audiopolicy.h>
#include <wrl/client.h>
#elif defined(__APPLE__)
#include <unistd.h>
#endif

#if defined(_WIN32)
std::wstring audioEndpointName(IMMDevice* device) {
  using Microsoft::WRL::ComPtr;
  ComPtr<IPropertyStore> properties;
  if (device == nullptr || FAILED(device->OpenPropertyStore(STGM_READ, &properties))) return {};
  PROPVARIANT value;
  PropVariantInit(&value);
  const auto result = properties->GetValue(PKEY_Device_FriendlyName, &value);
  std::wstring name;
  if (SUCCEEDED(result) && value.vt == VT_LPWSTR && value.pwszVal != nullptr)
    name = value.pwszVal;
  PropVariantClear(&value);
  return name;
}

bool isSilentVirtualEndpoint(std::wstring name) {
  std::transform(name.begin(), name.end(), name.begin(),
                 [](const wchar_t value) { return static_cast<wchar_t>(std::towlower(value)); });
  return name.find(L"elgato virtual audio") != std::wstring::npos ||
         name.find(L"vb-audio virtual cable") != std::wstring::npos ||
         name.find(L"cable input") != std::wstring::npos;
}

bool discordBridgeSessionIsSafe() {
  using Microsoft::WRL::ComPtr;
  ComPtr<IMMDeviceEnumerator> deviceEnumerator;
  if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                              IID_PPV_ARGS(&deviceEnumerator)))) return false;
  ComPtr<IMMDeviceCollection> devices;
  if (FAILED(deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices)))
    return false;
  UINT deviceCount {};
  if (FAILED(devices->GetCount(&deviceCount))) return false;
  bool virtualEndpointAvailable {};
  for (UINT deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex) {
    ComPtr<IMMDevice> device;
    if (FAILED(devices->Item(deviceIndex, &device))) continue;
    const auto endpointIsVirtual = isSilentVirtualEndpoint(audioEndpointName(device.Get()));
    virtualEndpointAvailable = virtualEndpointAvailable || endpointIsVirtual;
    ComPtr<IAudioSessionManager2> manager;
    if (FAILED(device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(manager.GetAddressOf())))) continue;
    ComPtr<IAudioSessionEnumerator> sessions;
    if (FAILED(manager->GetSessionEnumerator(&sessions))) continue;
    int count {};
    if (FAILED(sessions->GetCount(&count))) continue;
    for (int index = 0; index < count; ++index) {
      ComPtr<IAudioSessionControl> control;
      ComPtr<IAudioSessionControl2> control2;
      if (FAILED(sessions->GetSession(index, &control)) || FAILED(control.As(&control2))) continue;
      DWORD processId {};
      if (FAILED(control2->GetProcessId(&processId)) || processId != GetCurrentProcessId()) continue;
      LPWSTR displayName {};
      const auto gotName = SUCCEEDED(control->GetDisplayName(&displayName));
      const auto matches = gotName && displayName != nullptr &&
                           std::wstring_view(displayName) == L"DAS Discord Bridge";
      if (displayName != nullptr) CoTaskMemFree(displayName);
      if (!matches) continue;
      const GUID sessionId {0x90a5616e, 0x70b8, 0x46d1,
                            {0xb2, 0x45, 0x3d, 0x39, 0x83, 0x48, 0x2e, 0xa7}};
      ComPtr<ISimpleAudioVolume> volume;
      if (FAILED(manager->GetSimpleAudioVolume(&sessionId, 0, &volume))) return false;
      BOOL muted {};
      if (FAILED(volume->GetMute(&muted))) return false;
      return endpointIsVirtual && muted == FALSE;
    }
  }
  // 対応する仮想出力がない場合は、セッションを一切作らないのが正しい動作。
  return !virtualEndpointAvailable;
}
#endif

int main(const int argc, char** argv) {
  juce::ScopedJuceInitialiser_GUI juce;
  if (argc != 2) {
    std::cerr << "VST3バンドルのパスが必要です\n";
    return 1;
  }

#if defined(_WIN32)
  const auto testSuffix = std::wstring(L"vst3-smoke-") + std::to_wstring(GetCurrentProcessId());
  SetEnvironmentVariableW(L"DAS_TEST_NAMESPACE", testSuffix.c_str());
  const auto obsMappingName = std::wstring(das::transport::kObsAudioMappingName) + L'.' + testSuffix;
#elif defined(__APPLE__)
  const auto testSuffix = std::string("vst3-smoke-") +
                          std::to_string(static_cast<unsigned long long>(getpid()));
  setenv("DAS_TEST_NAMESPACE", testSuffix.c_str(), 1);
  const auto obsMappingName = std::wstring(das::transport::kObsAudioMappingName) + L'.' +
                              std::wstring(testSuffix.begin(), testSuffix.end());
#else
  const auto obsMappingName = std::wstring(das::transport::kObsAudioMappingName);
#endif
  const auto bytes = das::transport::requiredBytes(das::transport::kAudioCapacityFrames,
                                                    das::transport::kAudioChannels);
  auto memory = das::transport::NamedSharedMemory::create(obsMappingName, bytes);
  if (!memory.isOpen()) return 2;
  if (!memory.alreadyExisted())
    das::transport::RingBuffer::initialize(memory.storage(),
                                           das::transport::kAudioCapacityFrames,
                                           das::transport::kAudioChannels,
                                           das::transport::kAudioSampleRate);
  das::transport::RingBuffer ring(memory.storage());
  if (!ring.isCompatible()) return 3;
  ring.discardAll();
  ring.notifyConsumer();
  juce::VST3PluginFormat format;
  juce::OwnedArray<juce::PluginDescription> descriptions;
  const auto bundle = juce::File(juce::String::fromUTF8(argv[1])).getFullPathName();
  format.findAllTypesForFile(descriptions, bundle);
  if (descriptions.size() != 1) {
    std::cerr << "DAS SendをVST3として検出できません: " << descriptions.size() << '\n';
    return 4;
  }

  juce::String error;
  auto plugin = format.createInstanceFromDescription(*descriptions[0], 44100.0, 512, error);
  if (!plugin) {
    std::cerr << "VST3を生成できません: " << error << '\n';
    return 5;
  }
  plugin->setPlayConfigDetails(2, 2, 44100.0, 512);
  plugin->prepareToPlay(44100.0, 512);
#if defined(_WIN32)
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  if (!discordBridgeSessionIsSafe()) {
    std::cerr << "Discord音声セッションを確認できません\n";
    return 6;
  }
#endif

  juce::AudioBuffer<float> audio(2, 512);
  juce::MidiBuffer midi;
  auto duplicate = format.createInstanceFromDescription(*descriptions[0], 44100.0, 512, error);
  if (!duplicate) {
    std::cerr << "2個目のDAS Sendを生成できません: " << error << '\n';
    return 7;
  }
  duplicate->setPlayConfigDetails(2, 2, 44100.0, 512);
  duplicate->prepareToPlay(44100.0, 512);
  audio.clear();
  duplicate->processBlock(audio, midi);
  if (ring.availableToRead() != 0) {
    std::cerr << "2個目のDAS Sendが重複送信しました\n";
    return 8;
  }
  duplicate->releaseResources();
  duplicate.reset();

  double phase {};
  for (int block = 0; block < 100; ++block) {
    for (int sample = 0; sample < audio.getNumSamples(); ++sample) {
      const auto value = static_cast<float>(0.25 * std::sin(phase));
      phase += 2.0 * std::numbers::pi * 440.0 / 44100.0;
      audio.setSample(0, sample, value);
      audio.setSample(1, sample, value);
    }
    plugin->processBlock(audio, midi);
  }
  plugin->releaseResources();

  const auto available = ring.availableToRead();
  if (available < 54500 || available > 56500) {
    std::cerr << "44.1→48 kHz変換後のフレーム数が不正です: " << available << '\n';
    return 9;
  }
  std::vector<float> received(static_cast<std::size_t>(available) * 2);
  if (ring.read(received, available) != available) return 10;
  bool nonZero {};
  for (std::size_t frame = 0; frame < available; ++frame) {
    nonZero = nonZero || std::abs(received[frame * 2]) > 0.001F;
    if (received[frame * 2] != received[frame * 2 + 1]) return 11;
  }
  if (!nonZero) return 12;
  return 0;
}
