// SPDX-License-Identifier: GPL-3.0-or-later
#include <das/transport/NamedSharedMemory.h>
#include <das/transport/RingBuffer.h>

#include <obs-module.h>
#include <util/platform.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <thread>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("das-obs-source", "ja-JP")
MODULE_EXPORT const char* obs_module_description(void) {
  return "DAS SendからDAW音声を直接受信します";
}

namespace {
constexpr std::uint32_t kBlockFrames = 480;
constexpr std::uint32_t kMaxBufferedFrames = 4800;

struct DasObsSource final {
  explicit DasObsSource(obs_source_t* owner) : source(owner) {
    const auto bytes = das::transport::requiredBytes(das::transport::kAudioCapacityFrames,
                                                      das::transport::kAudioChannels);
    memory = das::transport::NamedSharedMemory::create(das::transport::kObsAudioMappingName, bytes);
    if (memory.isOpen()) {
      if (!memory.alreadyExisted())
        das::transport::RingBuffer::initialize(memory.storage(),
                                               das::transport::kAudioCapacityFrames,
                                               das::transport::kAudioChannels,
                                               das::transport::kAudioSampleRate);
      ring = std::make_unique<das::transport::RingBuffer>(memory.storage());
      if (!ring->isCompatible()) ring.reset();
      else ring->discardAll();
    }
    worker = std::thread([this] { run(); });
  }

  ~DasObsSource() {
    stopping.store(true, std::memory_order_release);
    if (worker.joinable()) worker.join();
  }

  void run() {
    while (!stopping.load(std::memory_order_acquire)) {
      const auto available = ring ? ring->availableToRead() : 0;
      if (available > kMaxBufferedFrames) {
        ring->discardAll();
        continue;
      }
      if (!ring || available < kBlockFrames) {
        if (ring) ring->notifyConsumer();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }
      ring->notifyConsumer();
      const auto read = ring->read(interleaved, kBlockFrames);
      if (read != kBlockFrames) continue;
      for (std::uint32_t frame = 0; frame < kBlockFrames; ++frame) {
        left[frame] = interleaved[frame * 2];
        right[frame] = interleaved[frame * 2 + 1];
      }
      obs_source_audio audio {};
      audio.data[0] = reinterpret_cast<std::uint8_t*>(left.data());
      audio.data[1] = reinterpret_cast<std::uint8_t*>(right.data());
      audio.frames = kBlockFrames;
      audio.speakers = SPEAKERS_STEREO;
      audio.samples_per_sec = das::transport::kAudioSampleRate;
      audio.format = AUDIO_FORMAT_FLOAT_PLANAR;
      audio.timestamp = os_gettime_ns();
      obs_source_output_audio(source, &audio);
    }
  }

  obs_source_t* source {};
  das::transport::NamedSharedMemory memory;
  std::unique_ptr<das::transport::RingBuffer> ring;
  std::atomic<bool> stopping {};
  std::thread worker;
  std::array<float, kBlockFrames * 2> interleaved {};
  std::array<float, kBlockFrames> left {};
  std::array<float, kBlockFrames> right {};
};

const char* sourceName(void*) { return "DAS Audio（DAW）"; }

void* createSource(obs_data_t*, obs_source_t* source) {
  try {
    return new DasObsSource(source);
  } catch (...) {
    return nullptr;
  }
}

void destroySource(void* data) { delete static_cast<DasObsSource*>(data); }

obs_source_info sourceInfo {
    .id = "das_audio_source",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE,
    .get_name = sourceName,
    .create = createSource,
    .destroy = destroySource,
};
} // namespace

bool obs_module_load(void) {
  obs_register_source(&sourceInfo);
  return true;
}
