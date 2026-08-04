// SPDX-License-Identifier: MIT
#include <das/transport/NamedSharedMemory.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <limits>
#include <string>
#include <utility>

namespace das::transport {
namespace {

std::wstring terminatedName(const std::wstring_view name) {
  return {name.begin(), name.end()};
}

} // namespace

NamedSharedMemory::NamedSharedMemory(void* const mapping, void* const view,
                                     const std::size_t bytes,
                                     const bool alreadyExisted) noexcept
    : mapping_(mapping), view_(view), bytes_(bytes), alreadyExisted_(alreadyExisted) {}

NamedSharedMemory::~NamedSharedMemory() noexcept { close(); }

NamedSharedMemory::NamedSharedMemory(NamedSharedMemory&& other) noexcept {
  *this = std::move(other);
}

NamedSharedMemory& NamedSharedMemory::operator=(NamedSharedMemory&& other) noexcept {
  if (this != &other) {
    close();
    mapping_ = std::exchange(other.mapping_, nullptr);
    view_ = std::exchange(other.view_, nullptr);
    bytes_ = std::exchange(other.bytes_, 0);
    alreadyExisted_ = std::exchange(other.alreadyExisted_, false);
  }
  return *this;
}

NamedSharedMemory NamedSharedMemory::create(const std::wstring_view name,
                                            const std::size_t bytes) noexcept {
  if (name.empty() || bytes == 0) return {};
  const auto stableName = terminatedName(name);
  const auto size = static_cast<unsigned long long>(bytes);
  const auto high = static_cast<DWORD>(size >> 32U);
  const auto low = static_cast<DWORD>(size & std::numeric_limits<DWORD>::max());
  auto* const mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                           high, low, stableName.c_str());
  if (mapping == nullptr) return {};
  const bool existed = GetLastError() == ERROR_ALREADY_EXISTS;
  auto* const view = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
  if (view == nullptr) {
    CloseHandle(mapping);
    return {};
  }
  return {mapping, view, bytes, existed};
}

NamedSharedMemory NamedSharedMemory::open(const std::wstring_view name,
                                          const std::size_t bytes) noexcept {
  if (name.empty() || bytes == 0) return {};
  const auto stableName = terminatedName(name);
  auto* const mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, stableName.c_str());
  if (mapping == nullptr) return {};
  auto* const view = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
  if (view == nullptr) {
    CloseHandle(mapping);
    return {};
  }
  return {mapping, view, bytes, true};
}

std::span<std::byte> NamedSharedMemory::storage() noexcept {
  return {static_cast<std::byte*>(view_), isOpen() ? bytes_ : 0};
}

std::span<const std::byte> NamedSharedMemory::storage() const noexcept {
  return {static_cast<const std::byte*>(view_), isOpen() ? bytes_ : 0};
}

void NamedSharedMemory::close() noexcept {
  if (view_ != nullptr) UnmapViewOfFile(std::exchange(view_, nullptr));
  if (mapping_ != nullptr) CloseHandle(std::exchange(mapping_, nullptr));
  bytes_ = 0;
  alreadyExisted_ = false;
}

} // namespace das::transport
