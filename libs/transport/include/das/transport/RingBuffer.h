// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace das::transport {

inline constexpr std::uint32_t kProtocolMagic = 0x44534153; // "DSAS"
inline constexpr std::uint16_t kProtocolMajor = 1;
inline constexpr std::uint16_t kProtocolMinor = 1;
inline constexpr std::uint32_t kAudioSampleRate = 48000;
inline constexpr std::uint16_t kAudioChannels = 2;
inline constexpr std::uint32_t kAudioCapacityFrames = 96000;
inline constexpr std::wstring_view kDefaultAudioMappingName =
    L"Local\\DawAudioStreamer.Audio.v1";
inline constexpr std::wstring_view kObsAudioMappingName =
    L"Local\\DawAudioStreamer.OBS.v1";
inline constexpr std::wstring_view kDiscordAudioMappingName =
    L"Local\\DawAudioStreamer.Discord.v1";

struct alignas(64) RingHeader final {
  std::uint32_t magic {kProtocolMagic};
  std::uint16_t major {kProtocolMajor};
  std::uint16_t minor {kProtocolMinor};
  std::uint32_t capacityFrames {};
  std::uint16_t channels {};
  std::uint16_t reserved {};
  std::uint32_t sampleRate {48000};
  std::uint32_t reserved2 {};
  std::atomic<std::uint64_t> writePosition {};
  std::atomic<std::uint64_t> readPosition {};
  std::atomic<std::uint64_t> droppedFrames {};
  std::atomic<std::uint64_t> producerHeartbeat {};
  std::atomic<std::uint64_t> consumerHeartbeat {};
};

static_assert(sizeof(RingHeader) == 64);
static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "転送にはロックフリーの64-bit atomicが必要です");

[[nodiscard]] std::size_t requiredBytes(std::uint32_t capacityFrames,
                                        std::uint16_t channels) noexcept;

class RingBuffer final {
public:
  static bool initialize(std::span<std::byte> storage,
                         std::uint32_t capacityFrames,
                         std::uint16_t channels,
                         std::uint32_t sampleRate = 48000) noexcept;

  explicit RingBuffer(std::span<std::byte> storage) noexcept;
  [[nodiscard]] bool isCompatible() const noexcept;
  [[nodiscard]] std::uint32_t availableToRead() const noexcept;
  [[nodiscard]] std::uint32_t availableToWrite() const noexcept;
  [[nodiscard]] std::uint32_t write(std::span<const float> interleaved,
                                    std::uint32_t frames) noexcept;
  [[nodiscard]] std::uint32_t read(std::span<float> interleaved,
                                   std::uint32_t frames) noexcept;
  void discardAll() noexcept;
  void notifyProducer() noexcept;
  void notifyConsumer() noexcept;
  [[nodiscard]] std::uint64_t producerHeartbeat() const noexcept;
  [[nodiscard]] std::uint64_t consumerHeartbeat() const noexcept;
  [[nodiscard]] const RingHeader* header() const noexcept { return header_; }

private:
  RingHeader* header_ {};
  float* data_ {};
  std::size_t storageBytes_ {};
};

} // namespace das::transport
