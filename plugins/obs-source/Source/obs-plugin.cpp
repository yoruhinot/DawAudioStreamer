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
#include <mutex>
#include <thread>
#include <vector>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("das-obs-source", "en-US")
MODULE_EXPORT const char* obs_module_description(void) {
  return "DAS SendからDAW音声を直接受信します";
}

namespace {
constexpr std::uint32_t kBlockFrames = 480;
constexpr std::uint32_t kMaxBufferedFrames = 4800;

// OBS側のリングは単一コンシューマーなので、DAS Audioソースが複数あっても
// 読み出しはプロセス内で1スレッドにまとめ、同じ音声を全ソースへ配る。
// ソースごとに読むとブロックを奪い合い、途切れとクリックノイズになる。
class SharedReceiver final {
public:
  void add(obs_source_t* source) {
    const std::lock_guard lifecycle(lifecycleMutex_);
    {
      const std::lock_guard lock(sourcesMutex_);
      sources_.push_back(source);
    }
    if (!worker_.joinable()) start();
  }

  void remove(obs_source_t* source) {
    const std::lock_guard lifecycle(lifecycleMutex_);
    bool empty {};
    {
      const std::lock_guard lock(sourcesMutex_);
      std::erase(sources_, source);
      empty = sources_.empty();
    }
    if (empty) stop();
  }

  ~SharedReceiver() { stop(); }

private:
  void start() {
    const auto bytes = das::transport::requiredBytes(das::transport::kAudioCapacityFrames,
                                                      das::transport::kAudioChannels);
    memory_ = das::transport::NamedSharedMemory::create(das::transport::kObsAudioMappingName, bytes);
    if (memory_.isOpen()) {
      if (!memory_.alreadyExisted())
        das::transport::RingBuffer::initialize(memory_.storage(),
                                               das::transport::kAudioCapacityFrames,
                                               das::transport::kAudioChannels,
                                               das::transport::kAudioSampleRate);
      ring_ = std::make_unique<das::transport::RingBuffer>(memory_.storage());
      if (!ring_->isCompatible()) ring_.reset();
      else ring_->discardAll();
    }
    stopping_.store(false, std::memory_order_release);
    worker_ = std::thread([this] { run(); });
  }

  void stop() {
    stopping_.store(true, std::memory_order_release);
    if (worker_.joinable()) worker_.join();
    ring_.reset();
    memory_ = {};
  }

  void run() {
    while (!stopping_.load(std::memory_order_acquire)) {
      const auto available = ring_ ? ring_->availableToRead() : 0;
      if (available > kMaxBufferedFrames) {
        ring_->discardAll();
        continue;
      }
      if (!ring_ || available < kBlockFrames) {
        if (ring_) ring_->notifyConsumer();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }
      ring_->notifyConsumer();
      const auto read = ring_->read(interleaved_, kBlockFrames);
      if (read != kBlockFrames) continue;
      for (std::uint32_t frame = 0; frame < kBlockFrames; ++frame) {
        left_[frame] = interleaved_[frame * 2];
        right_[frame] = interleaved_[frame * 2 + 1];
      }
      obs_source_audio audio {};
      audio.data[0] = reinterpret_cast<std::uint8_t*>(left_.data());
      audio.data[1] = reinterpret_cast<std::uint8_t*>(right_.data());
      audio.frames = kBlockFrames;
      audio.speakers = SPEAKERS_STEREO;
      audio.samples_per_sec = das::transport::kAudioSampleRate;
      audio.format = AUDIO_FORMAT_FLOAT_PLANAR;
      audio.timestamp = os_gettime_ns();
      // destroyはremove()でここが終わるのを待つため、出力中のソースは解放されない。
      const std::lock_guard lock(sourcesMutex_);
      for (auto* source : sources_) obs_source_output_audio(source, &audio);
    }
  }

  std::mutex lifecycleMutex_;
  std::mutex sourcesMutex_;
  std::vector<obs_source_t*> sources_;
  das::transport::NamedSharedMemory memory_;
  std::unique_ptr<das::transport::RingBuffer> ring_;
  std::atomic<bool> stopping_ {};
  std::thread worker_;
  std::array<float, kBlockFrames * 2> interleaved_ {};
  std::array<float, kBlockFrames> left_ {};
  std::array<float, kBlockFrames> right_ {};
};

SharedReceiver& receiver() {
  static SharedReceiver instance;
  return instance;
}

struct DasObsSource final {
  explicit DasObsSource(obs_source_t* owner) : source(owner) { receiver().add(source); }
  ~DasObsSource() { receiver().remove(source); }

  obs_source_t* source {};
};

const char* sourceName(void*) { return obs_module_text("DASAudioSource"); }

void* createSource(obs_data_t*, obs_source_t* source) {
  try {
    return new DasObsSource(source);
  } catch (...) {
    return nullptr;
  }
}

void destroySource(void* data) { delete static_cast<DasObsSource*>(data); }

obs_properties_t* sourceProperties(void*) {
  auto* properties = obs_properties_create();
  obs_properties_add_text(properties, "setup_help", obs_module_text("SetupHelp"), OBS_TEXT_INFO);
  return properties;
}

obs_source_info sourceInfo {
    .id = "das_audio_source",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE | OBS_SOURCE_CAP_DONT_SHOW_PROPERTIES,
    .get_name = sourceName,
    .create = createSource,
    .destroy = destroySource,
    .get_properties = sourceProperties,
};
} // namespace

bool obs_module_load(void) {
  obs_register_source(&sourceInfo);
  return true;
}
