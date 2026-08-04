// SPDX-License-Identifier: AGPL-3.0-only
#include "DiscordBridge.h"
#include "VirtualAudioEndpoint.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <audiopolicy.h>
#include <ksmedia.h>
#include <wrl/client.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <span>
#include <string>
#include <vector>

#if defined(_WIN32)
namespace {
using Microsoft::WRL::ComPtr;

class AdaptiveRingReader final {
public:
  explicit AdaptiveRingReader(das::transport::RingBuffer& ring)
      : ring_(ring), fifo_(kCapacityFrames * 2) {}

  std::uint32_t render(const std::span<float> output) noexcept {
    const auto requested = static_cast<std::uint32_t>(output.size() / 2);
    std::fill(output.begin(), output.end(), 0.0F);
    pull();
    if (!primed_) {
      if (fifoFrames_ < kTargetFrames) return 0;
      primed_ = true;
      position_ = 1.0;
    }

    const auto buffered = static_cast<double>(fifoFrames_) - position_;
    const auto normalisedError = (buffered - static_cast<double>(kTargetFrames)) /
                                 static_cast<double>(kTargetFrames);
    const auto ratio = std::clamp(1.0 + normalisedError * 0.004, 0.997, 1.003);

    std::uint32_t produced {};
    for (; produced < requested; ++produced) {
      const auto index = static_cast<std::size_t>(position_);
      if (index < 1 || index + 2 >= fifoFrames_) break;
      const auto fraction = static_cast<float>(position_ - static_cast<double>(index));
      for (std::size_t channel = 0; channel < 2; ++channel) {
        const auto p0 = fifo_[(index - 1) * 2 + channel];
        const auto p1 = fifo_[index * 2 + channel];
        const auto p2 = fifo_[(index + 1) * 2 + channel];
        const auto p3 = fifo_[(index + 2) * 2 + channel];
        output[static_cast<std::size_t>(produced) * 2 + channel] =
            p1 + 0.5F * fraction *
                (p2 - p0 + fraction *
                    (2.0F * p0 - 5.0F * p1 + 4.0F * p2 - p3 +
                     fraction * (3.0F * (p1 - p2) + p3 - p0)));
      }
      position_ += ratio;
    }

    const auto consumed = position_ >= 2.0
        ? std::min(fifoFrames_, static_cast<std::size_t>(position_) - 1)
        : 0;
    if (consumed > 0) {
      const auto remaining = fifoFrames_ - consumed;
      std::memmove(fifo_.data(), fifo_.data() + consumed * 2,
                   remaining * 2 * sizeof(float));
      fifoFrames_ = remaining;
      position_ -= static_cast<double>(consumed);
    }

    if (produced < requested) {
      primed_ = false;
      fifoFrames_ = 0;
      position_ = 1.0;
      ++corrections_;
    }
    return produced;
  }

  [[nodiscard]] std::uint64_t corrections() const noexcept { return corrections_; }

private:
  void pull() noexcept {
    ring_.notifyConsumer();
    const auto writable = static_cast<std::uint32_t>(kCapacityFrames - fifoFrames_);
    const auto wanted = std::min(writable, ring_.availableToRead());
    if (wanted > 0) {
      const auto destination = std::span<float>(fifo_.data() + fifoFrames_ * 2,
                                                static_cast<std::size_t>(wanted) * 2);
      fifoFrames_ += ring_.read(destination, wanted);
    }
    if (fifoFrames_ == kCapacityFrames && ring_.availableToRead() > 0) {
      const auto discard = fifoFrames_ - kTargetFrames;
      std::memmove(fifo_.data(), fifo_.data() + discard * 2,
                   kTargetFrames * 2 * sizeof(float));
      fifoFrames_ = kTargetFrames;
      position_ = 1.0;
      ring_.discardAll();
      ++corrections_;
    }
  }

  static constexpr std::size_t kTargetFrames = 1920;  // 40 ms at 48 kHz
  static constexpr std::size_t kCapacityFrames = 8192;
  das::transport::RingBuffer& ring_;
  std::vector<float> fifo_;
  std::size_t fifoFrames_ {};
  double position_ {1.0};
  std::uint64_t corrections_ {};
  bool primed_ {};
};
} // namespace
#endif

DiscordBridge::DiscordBridge(das::transport::RingBuffer& ring) noexcept : ring_(ring) {}

DiscordBridge::~DiscordBridge() { stop(); }

void DiscordBridge::start() {
  if (worker_.joinable()) return;
  stopping_.store(false, std::memory_order_release);
  state_.store(State::starting, std::memory_order_relaxed);
#if defined(_WIN32)
  wakeEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (wakeEvent_ == nullptr) {
    state_.store(State::failed, std::memory_order_relaxed);
    return;
  }
#endif
  worker_ = std::thread([this] { run(); });
}

void DiscordBridge::stop() noexcept {
  stopping_.store(true, std::memory_order_release);
#if defined(_WIN32)
  if (wakeEvent_ != nullptr) SetEvent(static_cast<HANDLE>(wakeEvent_));
#endif
  if (worker_.joinable()) worker_.join();
#if defined(_WIN32)
  if (wakeEvent_ != nullptr) CloseHandle(static_cast<HANDLE>(wakeEvent_));
#endif
  wakeEvent_ = nullptr;
  state_.store(State::stopped, std::memory_order_relaxed);
}

void DiscordBridge::run() noexcept {
#if defined(_WIN32)
  const auto comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  const auto uninitialize = SUCCEEDED(comResult);
  if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
    state_.store(State::failed, std::memory_order_relaxed);
    return;
  }
  while (!stopping_.load(std::memory_order_acquire)) {
    state_.store(State::starting, std::memory_order_relaxed);
    static_cast<void>(runAudioClient());
    if (!stopping_.load(std::memory_order_acquire))
      WaitForSingleObject(static_cast<HANDLE>(wakeEvent_), 1000);
  }
  if (uninitialize) CoUninitialize();
#else
  state_.store(State::failed, std::memory_order_relaxed);
#endif
}

bool DiscordBridge::runAudioClient() noexcept {
#if !defined(_WIN32)
  return false;
#else
  using Microsoft::WRL::ComPtr;
  ComPtr<IMMDeviceEnumerator> enumerator;
  auto result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                 IID_PPV_ARGS(&enumerator));
  if (FAILED(result)) {
    state_.store(State::failed, std::memory_order_relaxed);
    return false;
  }

  ComPtr<IMMDevice> endpoint;
  if (!das::discord::findSupportedVirtualAudioEndpoint(enumerator.Get(),
                                                        endpoint.GetAddressOf())) {
    // Rendering to a physical fallback makes the ASIO signal audible twice.
    // Refuse it and guide the user to a supported silent virtual endpoint.
    state_.store(State::virtualOutputRequired, std::memory_order_relaxed);
    return false;
  }

  ComPtr<IAudioClient> audioClient;
  result = endpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                              reinterpret_cast<void**>(audioClient.GetAddressOf()));
  if (FAILED(result)) {
    state_.store(State::failed, std::memory_order_relaxed);
    return false;
  }

  WAVEFORMATEXTENSIBLE format {};
  format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  format.Format.nChannels = das::transport::kAudioChannels;
  format.Format.nSamplesPerSec = das::transport::kAudioSampleRate;
  format.Format.wBitsPerSample = 32;
  format.Format.nBlockAlign = static_cast<WORD>(format.Format.nChannels * sizeof(float));
  format.Format.nAvgBytesPerSec = format.Format.nSamplesPerSec * format.Format.nBlockAlign;
  format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
  format.Samples.wValidBitsPerSample = 32;
  format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
  format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

  // Discord's process loopback captures this ordinary WASAPI render stream. It is
  // intentionally sent only to a silent virtual sink, never to physical speakers.
  const GUID sessionId {0x90a5616e, 0x70b8, 0x46d1,
                        {0xb2, 0x45, 0x3d, 0x39, 0x83, 0x48, 0x2e, 0xa7}};
  constexpr DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                          AUDCLNT_STREAMFLAGS_NOPERSIST |
                          AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                          AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
  constexpr REFERENCE_TIME bufferDuration = 20 * 10'000;
  result = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, bufferDuration, 0,
                                   &format.Format, &sessionId);
  if (FAILED(result)) {
    state_.store(State::failed, std::memory_order_relaxed);
    return false;
  }
  result = audioClient->SetEventHandle(static_cast<HANDLE>(wakeEvent_));
  if (FAILED(result)) {
    state_.store(State::failed, std::memory_order_relaxed);
    return false;
  }

  ComPtr<IAudioRenderClient> renderer;
  ComPtr<ISimpleAudioVolume> volume;
  ComPtr<IAudioSessionControl> session;
  if (FAILED(audioClient->GetService(IID_PPV_ARGS(&renderer))) ||
      FAILED(audioClient->GetService(IID_PPV_ARGS(&volume))) ||
      FAILED(audioClient->GetService(IID_PPV_ARGS(&session)))) {
    state_.store(State::failed, std::memory_order_relaxed);
    return false;
  }
  static_cast<void>(session->SetDisplayName(L"DAS Discord Bridge", nullptr));
  static_cast<void>(volume->SetMasterVolume(1.0F, &sessionId));
  result = volume->SetMute(FALSE, &sessionId);
  if (FAILED(result)) {
    state_.store(State::failed, std::memory_order_relaxed);
    return false;
  }

  UINT32 bufferFrames {};
  if (FAILED(audioClient->GetBufferSize(&bufferFrames))) {
    state_.store(State::failed, std::memory_order_relaxed);
    return false;
  }
  BYTE* initialData {};
  if (SUCCEEDED(renderer->GetBuffer(bufferFrames, &initialData)))
    static_cast<void>(renderer->ReleaseBuffer(bufferFrames, AUDCLNT_BUFFERFLAGS_SILENT));
  ring_.discardAll();
  AdaptiveRingReader adaptiveReader(ring_);
  result = audioClient->Start();
  if (FAILED(result)) {
    state_.store(State::failed, std::memory_order_relaxed);
    return false;
  }
  state_.store(State::ready, std::memory_order_relaxed);

  bool healthy = true;
  while (!stopping_.load(std::memory_order_acquire) && healthy) {
    const auto wait = WaitForSingleObject(static_cast<HANDLE>(wakeEvent_), 250);
    if (wait == WAIT_TIMEOUT) continue;
    if (wait != WAIT_OBJECT_0) break;
    if (stopping_.load(std::memory_order_acquire)) break;
    UINT32 padding {};
    if (FAILED(audioClient->GetCurrentPadding(&padding))) break;
    const auto availableFrames = bufferFrames - std::min(bufferFrames, padding);
    if (availableFrames == 0) continue;
    BYTE* data {};
    if (FAILED(renderer->GetBuffer(availableFrames, &data))) break;
    auto output = std::span<float>(reinterpret_cast<float*>(data),
                                   static_cast<std::size_t>(availableFrames) * 2);
    const auto read = adaptiveReader.render(output);
    if (FAILED(renderer->ReleaseBuffer(availableFrames, 0))) healthy = false;
    renderedFrames_.fetch_add(read, std::memory_order_relaxed);
    bufferCorrections_.store(adaptiveReader.corrections(), std::memory_order_relaxed);
  }
  static_cast<void>(audioClient->Stop());
  if (!stopping_.load(std::memory_order_acquire))
    state_.store(State::failed, std::memory_order_relaxed);
  return healthy;
#endif
}
