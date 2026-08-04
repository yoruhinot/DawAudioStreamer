// SPDX-License-Identifier: MIT
#include <das/transport/NamedSharedMemory.h>
#include <das/transport/RingBuffer.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr std::uint32_t kCapacity = 1024;
constexpr std::uint16_t kChannels = 2;
constexpr std::uint32_t kTotalFrames = 200000;

int runProducer(const std::wstring& name) {
  const auto bytes = das::transport::requiredBytes(kCapacity, kChannels);
  auto memory = das::transport::NamedSharedMemory::open(name, bytes);
  das::transport::RingBuffer ring(memory.storage());
  if (!memory.isOpen() || !ring.isCompatible()) return 10;

  std::array<float, kChannels> frame {};
  for (std::uint32_t sequence = 0; sequence < kTotalFrames;) {
    frame.fill(static_cast<float>(sequence));
    if (ring.write(frame, 1) == 1) ++sequence;
    else SwitchToThread();
  }
  return 0;
}

int runConsumer(const std::wstring& name) {
  const auto bytes = das::transport::requiredBytes(kCapacity, kChannels);
  auto memory = das::transport::NamedSharedMemory::open(name, bytes);
  das::transport::RingBuffer ring(memory.storage());
  if (!memory.isOpen() || !ring.isCompatible()) return 20;

  std::array<float, kChannels> frame {};
  for (std::uint32_t sequence = 0; sequence < kTotalFrames;) {
    if (ring.read(frame, 1) == 0) {
      SwitchToThread();
      continue;
    }
    for (const auto sample : frame)
      if (sample != static_cast<float>(sequence)) return 21;
    ++sequence;
  }
  return 0;
}

bool startChild(const std::wstring& executable, const wchar_t* role,
                const std::wstring& name, PROCESS_INFORMATION& process) {
  std::wstring command = L"\"" + executable + L"\" " + role + L" \"" + name + L"\"";
  STARTUPINFOW startup {};
  startup.cb = sizeof(startup);
  return CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process) != FALSE;
}

int runCoordinator(const std::wstring& executable) {
  const auto name = L"Local\\DawAudioStreamer.IpcTest." + std::to_wstring(GetCurrentProcessId());
  const auto bytes = das::transport::requiredBytes(kCapacity, kChannels);
  auto memory = das::transport::NamedSharedMemory::create(name, bytes);
  if (!memory.isOpen() || memory.alreadyExisted()) {
    std::cerr << "名前付き共有メモリを新規作成できませんでした\n";
    return 1;
  }
  if (!das::transport::RingBuffer::initialize(memory.storage(), kCapacity, kChannels)) {
    std::cerr << "共有リングを初期化できませんでした\n";
    return 2;
  }

  PROCESS_INFORMATION producer {};
  PROCESS_INFORMATION consumer {};
  if (!startChild(executable, L"--producer", name, producer) ||
      !startChild(executable, L"--consumer", name, consumer)) {
    std::cerr << "子プロセスを起動できませんでした\n";
    return 3;
  }
  CloseHandle(producer.hThread);
  CloseHandle(consumer.hThread);
  const std::array<HANDLE, 2> processes {producer.hProcess, consumer.hProcess};
  const auto wait = WaitForMultipleObjects(static_cast<DWORD>(processes.size()),
                                           processes.data(), TRUE, 15000);
  DWORD producerCode = 1;
  DWORD consumerCode = 1;
  GetExitCodeProcess(producer.hProcess, &producerCode);
  GetExitCodeProcess(consumer.hProcess, &consumerCode);
  CloseHandle(producer.hProcess);
  CloseHandle(consumer.hProcess);
  if (wait == WAIT_TIMEOUT || producerCode != 0 || consumerCode != 0) {
    std::cerr << "プロセス間転送に失敗しました: producer=" << producerCode
              << " consumer=" << consumerCode << '\n';
    return 4;
  }
  return 0;
}
} // namespace

int wmain(const int argc, wchar_t** argv) {
  if (argc == 3 && std::wstring_view(argv[1]) == L"--producer") return runProducer(argv[2]);
  if (argc == 3 && std::wstring_view(argv[1]) == L"--consumer") return runConsumer(argv[2]);

  std::vector<wchar_t> path(32768);
  const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
  if (length == 0 || length == path.size()) return 5;
  return runCoordinator(std::wstring(path.data(), length));
}
