// SPDX-License-Identifier: MIT
#include <das/transport/RingBuffer.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

namespace {
int failures {};
void check(const bool condition, const char* message) {
  if (!condition) { std::cerr << message << '\n'; ++failures; }
}
}

int main() {
  constexpr std::uint32_t capacity = 256;
  constexpr std::uint16_t channels = 2;
  std::vector<std::byte> raw(das::transport::requiredBytes(capacity, channels) + 63);
  const auto address = reinterpret_cast<std::uintptr_t>(raw.data());
  const auto offset = (alignof(das::transport::RingHeader) - address % alignof(das::transport::RingHeader)) % alignof(das::transport::RingHeader);
  std::span<std::byte> storage(raw.data() + offset, raw.size() - offset);
  check(das::transport::RingBuffer::initialize(storage, capacity, channels), "リングの初期化に失敗しました");
  das::transport::RingBuffer ring(storage);
  check(ring.isCompatible(), "互換性のあるリングが拒否されました");

  const std::array<float, 6> input {1, 2, 3, 4, 5, 6};
  std::array<float, 6> output {};
  check(ring.write(input, 3) == 3, "基本書き込みに失敗しました");
  check(ring.read(output, 3) == 3, "基本読み取りに失敗しました");
  check(input == output, "往復転送でサンプルが破損しました");

  constexpr std::uint32_t total = 200000;
  std::atomic<bool> corrupt {};
  std::thread producer([&] {
    std::array<float, 2> frame {};
    for (std::uint32_t sequence = 0; sequence < total;) {
      frame = {static_cast<float>(sequence), static_cast<float>(sequence)};
      if (ring.write(frame, 1) == 1) ++sequence;
    }
  });
  std::thread consumer([&] {
    std::array<float, 2> frame {};
    for (std::uint32_t sequence = 0; sequence < total;) {
      if (ring.read(frame, 1) == 1) {
        if (frame[0] != static_cast<float>(sequence) || frame[1] != frame[0]) corrupt = true;
        ++sequence;
      }
    }
  });
  producer.join();
  consumer.join();
  check(!corrupt, "SPSCストレステストで破損を検出しました");

  std::vector<std::byte> small(sizeof(das::transport::RingHeader));
  das::transport::RingBuffer invalid(small);
  check(!invalid.isCompatible(), "未初期化領域が受理されました");
  return failures == 0 ? 0 : 1;
}
