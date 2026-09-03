// SPDX-License-Identifier: MIT
#include <das/transport/NamedSharedMemory.h>
#include <das/transport/RingBuffer.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace {
constexpr std::uint32_t kCapacity = 256;
constexpr std::uint16_t kChannels = 2;
constexpr std::uint32_t kTotalFrames = 20000;

std::wstring widen(const std::string_view text) {
  return {text.begin(), text.end()};
}

int runProducer(const std::wstring& name) {
  const auto bytes = das::transport::requiredBytes(kCapacity, kChannels);
  auto memory = das::transport::NamedSharedMemory::open(name, bytes);
  das::transport::RingBuffer ring(memory.storage());
  if (!memory.isOpen() || !ring.isCompatible()) return 10;

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
  std::array<float, kChannels> frame {};
  for (std::uint32_t sequence = 0; sequence < kTotalFrames;) {
    frame.fill(static_cast<float>(sequence));
    if (ring.write(frame, 1) == 1) ++sequence;
    else std::this_thread::yield();
    if (std::chrono::steady_clock::now() >= deadline) return 11;
  }
  return 0;
}

int runConsumer(const std::wstring& name) {
  const auto bytes = das::transport::requiredBytes(kCapacity, kChannels);
  auto memory = das::transport::NamedSharedMemory::open(name, bytes);
  das::transport::RingBuffer ring(memory.storage());
  if (!memory.isOpen() || !ring.isCompatible()) return 20;

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
  std::array<float, kChannels> frame {};
  for (std::uint32_t sequence = 0; sequence < kTotalFrames;) {
    if (ring.read(frame, 1) == 0) {
      std::this_thread::yield();
    } else {
      for (const auto sample : frame)
        if (sample != static_cast<float>(sequence)) return 21;
      ++sequence;
    }
    if (std::chrono::steady_clock::now() >= deadline) return 22;
  }
  return 0;
}

pid_t startChild(const char* executable, const char* role, const std::string& name) {
  const auto process = fork();
  if (process == 0) {
    execl(executable, executable, role, name.c_str(), nullptr);
    _exit(127);
  }
  return process;
}

int runCoordinator(const char* executable) {
  const auto name = std::string("Local\\DawAudioStreamer.IpcTest.") +
                    std::to_string(static_cast<unsigned long long>(getpid()));
  const auto wideName = widen(name);
  const auto bytes = das::transport::requiredBytes(kCapacity, kChannels);
  auto memory = das::transport::NamedSharedMemory::create(wideName, bytes);
  if (!memory.isOpen() || memory.alreadyExisted()) {
    std::cerr << "名前付き共有メモリを新規作成できませんでした\n";
    return 1;
  }
  if (!das::transport::RingBuffer::initialize(memory.storage(), kCapacity, kChannels)) {
    std::cerr << "共有リングを初期化できませんでした\n";
    return 2;
  }

  const auto producer = startChild(executable, "--producer", name);
  const auto consumer = startChild(executable, "--consumer", name);
  if (producer <= 0 || consumer <= 0) return 3;
  int producerStatus {};
  int consumerStatus {};
  if (waitpid(producer, &producerStatus, 0) != producer ||
      waitpid(consumer, &consumerStatus, 0) != consumer ||
      !WIFEXITED(producerStatus) || !WIFEXITED(consumerStatus) ||
      WEXITSTATUS(producerStatus) != 0 || WEXITSTATUS(consumerStatus) != 0) {
    std::cerr << "プロセス間転送に失敗しました\n";
    return 4;
  }
  return 0;
}
} // namespace

int main(const int argc, char** argv) {
  if (argc == 3 && std::string_view(argv[1]) == "--producer")
    return runProducer(widen(argv[2]));
  if (argc == 3 && std::string_view(argv[1]) == "--consumer")
    return runConsumer(widen(argv[2]));
  return runCoordinator(argv[0]);
}
