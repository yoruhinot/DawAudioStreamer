// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <das/transport/RingBuffer.h>

#include <atomic>
#include <cstdint>
#include <thread>

class DiscordBridge final {
public:
  enum class State : std::uint8_t {
    stopped,
    starting,
    ready,
    virtualOutputRequired,
    failed
  };

  explicit DiscordBridge(das::transport::RingBuffer& ring) noexcept;
  ~DiscordBridge();

  DiscordBridge(const DiscordBridge&) = delete;
  DiscordBridge& operator=(const DiscordBridge&) = delete;

  void start();
  void stop() noexcept;
  [[nodiscard]] State state() const noexcept { return state_.load(std::memory_order_relaxed); }
  [[nodiscard]] std::uint64_t renderedFrames() const noexcept {
    return renderedFrames_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::uint64_t bufferCorrections() const noexcept {
    return bufferCorrections_.load(std::memory_order_relaxed);
  }

private:
  void run() noexcept;
  [[nodiscard]] bool runAudioClient() noexcept;

  das::transport::RingBuffer& ring_;
  std::atomic<bool> stopping_ {};
  std::atomic<State> state_ {State::stopped};
  std::atomic<std::uint64_t> renderedFrames_ {};
  std::atomic<std::uint64_t> bufferCorrections_ {};
  std::thread worker_;
  void* wakeEvent_ {};
};
