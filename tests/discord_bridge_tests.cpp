// SPDX-License-Identifier: AGPL-3.0-only
#include "DiscordBridge.h"
#include "VirtualAudioEndpoint.h"

#include <das/transport/RingBuffer.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <thread>

int main() {
  alignas(64) std::array<std::byte, 64 + 8192 * 2 * sizeof(float)> storage {};
  if (!das::transport::RingBuffer::initialize(storage, 8192, 2)) return 1;
  das::transport::RingBuffer ring(storage);
  if (!ring.isCompatible()) return 2;

  DiscordBridge bridge(ring);
  if (bridge.state() != DiscordBridge::State::stopped) return 3;
  if (!das::discord::isSupportedVirtualAudioEndpointName(
          L"CABLE Input (VB-Audio Virtual Cable)")) return 4;
  if (!das::discord::isSupportedVirtualAudioEndpointName(L"Elgato Virtual Audio")) return 5;
  if (das::discord::isSupportedVirtualAudioEndpointName(L"Speakers (Audio Interface)")) return 6;

#if defined(_WIN32)
  std::array<float, 64 * 2> silence {};
  bridge.start();
  bool reachedTerminalState {};
  for (int attempt = 0; attempt < 400; ++attempt) {
    ring.notifyProducer();
    static_cast<void>(ring.write(silence, 64));
    const auto state = bridge.state();
    if (state == DiscordBridge::State::virtualOutputRequired) {
      // 仮想出力がないPCでは、物理出力へフォールバックしないことが安全要件。
      reachedTerminalState = bridge.renderedFrames() == 0;
      break;
    }
    if (state == DiscordBridge::State::ready && bridge.renderedFrames() > 0) {
      reachedTerminalState = true;
      break;
    }
    if (state == DiscordBridge::State::failed) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  if (!reachedTerminalState) return 7;
  bridge.stop();
  if (bridge.state() != DiscordBridge::State::stopped) return 8;
#endif
  return 0;
}
