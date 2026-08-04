// SPDX-License-Identifier: MIT
#include <das/transport/RingBuffer.h>

#include <algorithm>
#include <limits>
#include <new>

namespace das::transport {

std::size_t requiredBytes(const std::uint32_t capacityFrames,
                          const std::uint16_t channels) noexcept {
  if (capacityFrames == 0 || channels == 0)
    return 0;
  constexpr auto max = std::numeric_limits<std::size_t>::max();
  const auto samples = static_cast<std::size_t>(capacityFrames) * channels;
  if (samples > (max - sizeof(RingHeader)) / sizeof(float))
    return 0;
  return sizeof(RingHeader) + samples * sizeof(float);
}

bool RingBuffer::initialize(const std::span<std::byte> storage,
                            const std::uint32_t capacityFrames,
                            const std::uint16_t channels,
                            const std::uint32_t sampleRate) noexcept {
  const auto bytes = requiredBytes(capacityFrames, channels);
  if (bytes == 0 || storage.size() < bytes ||
      reinterpret_cast<std::uintptr_t>(storage.data()) % alignof(RingHeader) != 0)
    return false;

  auto* header = ::new (storage.data()) RingHeader {};
  header->capacityFrames = capacityFrames;
  header->channels = channels;
  header->sampleRate = sampleRate;
  return true;
}

RingBuffer::RingBuffer(const std::span<std::byte> storage) noexcept
    : storageBytes_(storage.size()) {
  if (storage.size() >= sizeof(RingHeader) &&
      reinterpret_cast<std::uintptr_t>(storage.data()) % alignof(RingHeader) == 0) {
    header_ = reinterpret_cast<RingHeader*>(storage.data());
    data_ = reinterpret_cast<float*>(storage.data() + sizeof(RingHeader));
  }
}

bool RingBuffer::isCompatible() const noexcept {
  if (header_ == nullptr || header_->magic != kProtocolMagic ||
      header_->major != kProtocolMajor || header_->channels == 0 ||
      header_->capacityFrames == 0)
    return false;
  return requiredBytes(header_->capacityFrames, header_->channels) <= storageBytes_;
}

std::uint32_t RingBuffer::availableToRead() const noexcept {
  if (!isCompatible()) return 0;
  const auto write = header_->writePosition.load(std::memory_order_acquire);
  const auto read = header_->readPosition.load(std::memory_order_relaxed);
  return static_cast<std::uint32_t>(std::min<std::uint64_t>(write - read, header_->capacityFrames));
}

std::uint32_t RingBuffer::availableToWrite() const noexcept {
  return isCompatible() ? header_->capacityFrames - availableToRead() : 0;
}

std::uint32_t RingBuffer::write(const std::span<const float> interleaved,
                                const std::uint32_t frames) noexcept {
  if (!isCompatible() || interleaved.size() < static_cast<std::size_t>(frames) * header_->channels)
    return 0;
  const auto writable = std::min(frames, availableToWrite());
  const auto writePosition = header_->writePosition.load(std::memory_order_relaxed);
  for (std::uint32_t frame = 0; frame < writable; ++frame) {
    const auto destinationFrame = (writePosition + frame) % header_->capacityFrames;
    for (std::uint16_t channel = 0; channel < header_->channels; ++channel)
      data_[destinationFrame * header_->channels + channel] =
          interleaved[static_cast<std::size_t>(frame) * header_->channels + channel];
  }
  header_->writePosition.store(writePosition + writable, std::memory_order_release);
  if (writable < frames)
    header_->droppedFrames.fetch_add(frames - writable, std::memory_order_relaxed);
  return writable;
}

std::uint32_t RingBuffer::read(const std::span<float> interleaved,
                               const std::uint32_t frames) noexcept {
  if (!isCompatible() || interleaved.size() < static_cast<std::size_t>(frames) * header_->channels)
    return 0;
  const auto readable = std::min(frames, availableToRead());
  const auto readPosition = header_->readPosition.load(std::memory_order_relaxed);
  for (std::uint32_t frame = 0; frame < readable; ++frame) {
    const auto sourceFrame = (readPosition + frame) % header_->capacityFrames;
    for (std::uint16_t channel = 0; channel < header_->channels; ++channel)
      interleaved[static_cast<std::size_t>(frame) * header_->channels + channel] =
          data_[sourceFrame * header_->channels + channel];
  }
  header_->readPosition.store(readPosition + readable, std::memory_order_release);
  return readable;
}

void RingBuffer::discardAll() noexcept {
  if (!isCompatible()) return;
  const auto write = header_->writePosition.load(std::memory_order_acquire);
  header_->readPosition.store(write, std::memory_order_release);
}

void RingBuffer::notifyProducer() noexcept {
  if (isCompatible()) header_->producerHeartbeat.fetch_add(1, std::memory_order_relaxed);
}

void RingBuffer::notifyConsumer() noexcept {
  if (isCompatible()) header_->consumerHeartbeat.fetch_add(1, std::memory_order_relaxed);
}

std::uint64_t RingBuffer::producerHeartbeat() const noexcept {
  return isCompatible() ? header_->producerHeartbeat.load(std::memory_order_relaxed) : 0;
}

std::uint64_t RingBuffer::consumerHeartbeat() const noexcept {
  return isCompatible() ? header_->consumerHeartbeat.load(std::memory_order_relaxed) : 0;
}

} // namespace das::transport
