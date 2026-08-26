// SPDX-License-Identifier: MIT
#include <das/transport/NamedSharedMemory.h>

#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace das::transport {
namespace {

std::string posixName(const std::wstring_view name) {
  // Darwin limits POSIX shared-memory names to 31 characters. Use a stable
  // per-user hash so the public transport names and test namespaces fit while
  // still resolving to the same object in independent processes.
  std::uint64_t hash = 14695981039346656037ULL;
  for (const auto value : name) {
    const auto codePoint = static_cast<std::uint32_t>(value);
    for (unsigned int shift = 0; shift < 32; shift += 8) {
      hash ^= (codePoint >> shift) & 0xffU;
      hash *= 1099511628211ULL;
    }
  }
  char result[31] {};
  std::snprintf(result, sizeof(result), "/das-%08x-%016llx",
                static_cast<unsigned int>(getuid()),
                static_cast<unsigned long long>(hash));
  return result;
}

bool ensureSize(const int handle, const std::size_t bytes) noexcept {
  struct stat status {};
  if (fstat(handle, &status) != 0) return false;
  if (status.st_size >= static_cast<off_t>(bytes)) return true;
  return ftruncate(handle, static_cast<off_t>(bytes)) == 0;
}

} // namespace

NamedSharedMemory::NamedSharedMemory(const std::intptr_t nativeHandle, void* const view,
                                     const std::size_t bytes,
                                     const bool alreadyExisted) noexcept
    : nativeHandle_(nativeHandle), view_(view), bytes_(bytes),
      alreadyExisted_(alreadyExisted) {}

NamedSharedMemory::~NamedSharedMemory() noexcept { close(); }

NamedSharedMemory::NamedSharedMemory(NamedSharedMemory&& other) noexcept {
  *this = std::move(other);
}

NamedSharedMemory& NamedSharedMemory::operator=(NamedSharedMemory&& other) noexcept {
  if (this != &other) {
    close();
    nativeHandle_ = std::exchange(other.nativeHandle_, -1);
    view_ = std::exchange(other.view_, nullptr);
    bytes_ = std::exchange(other.bytes_, 0);
    alreadyExisted_ = std::exchange(other.alreadyExisted_, false);
  }
  return *this;
}

NamedSharedMemory NamedSharedMemory::create(const std::wstring_view name,
                                            const std::size_t bytes) noexcept {
  if (name.empty() || bytes == 0) return {};
  const auto stableName = posixName(name);
  bool existed {};
  auto handle = shm_open(stableName.c_str(), O_RDWR | O_CREAT | O_EXCL,
                         S_IRUSR | S_IWUSR);
  if (handle < 0 && errno == EEXIST) {
    existed = true;
    handle = shm_open(stableName.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
  }
  if (handle < 0 || !ensureSize(handle, bytes)) {
    if (handle >= 0) ::close(handle);
    return {};
  }
  auto* const view = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
  if (view == MAP_FAILED) {
    ::close(handle);
    return {};
  }
  return {handle, view, bytes, existed};
}

NamedSharedMemory NamedSharedMemory::open(const std::wstring_view name,
                                          const std::size_t bytes) noexcept {
  if (name.empty() || bytes == 0) return {};
  const auto stableName = posixName(name);
  const auto handle = shm_open(stableName.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
  if (handle < 0) return {};
  struct stat status {};
  if (fstat(handle, &status) != 0 || status.st_size < static_cast<off_t>(bytes)) {
    ::close(handle);
    return {};
  }
  auto* const view = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
  if (view == MAP_FAILED) {
    ::close(handle);
    return {};
  }
  return {handle, view, bytes, true};
}

std::span<std::byte> NamedSharedMemory::storage() noexcept {
  return {static_cast<std::byte*>(view_), isOpen() ? bytes_ : 0};
}

std::span<const std::byte> NamedSharedMemory::storage() const noexcept {
  return {static_cast<const std::byte*>(view_), isOpen() ? bytes_ : 0};
}

void NamedSharedMemory::close() noexcept {
  if (view_ != nullptr) munmap(std::exchange(view_, nullptr), bytes_);
  if (nativeHandle_ != -1) ::close(static_cast<int>(std::exchange(nativeHandle_, -1)));
  bytes_ = 0;
  alreadyExisted_ = false;
}

} // namespace das::transport
