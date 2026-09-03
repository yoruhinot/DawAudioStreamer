// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace das::transport {

// OSの名前付き共有メモリを所有するRAIIラッパー。
// create/openは制御スレッドでだけ呼び、音声スレッドではstorage()だけを使用する。
class NamedSharedMemory final {
public:
  NamedSharedMemory() noexcept = default;
  ~NamedSharedMemory() noexcept;

  NamedSharedMemory(const NamedSharedMemory&) = delete;
  NamedSharedMemory& operator=(const NamedSharedMemory&) = delete;
  NamedSharedMemory(NamedSharedMemory&& other) noexcept;
  NamedSharedMemory& operator=(NamedSharedMemory&& other) noexcept;

  [[nodiscard]] static NamedSharedMemory create(std::wstring_view name,
                                                std::size_t bytes) noexcept;
  [[nodiscard]] static NamedSharedMemory open(std::wstring_view name,
                                              std::size_t bytes) noexcept;

  [[nodiscard]] bool isOpen() const noexcept { return view_ != nullptr; }
  [[nodiscard]] bool alreadyExisted() const noexcept { return alreadyExisted_; }
  [[nodiscard]] std::span<std::byte> storage() noexcept;
  [[nodiscard]] std::span<const std::byte> storage() const noexcept;

private:
  NamedSharedMemory(std::intptr_t nativeHandle, void* view, std::size_t bytes,
                    bool alreadyExisted) noexcept;
  void close() noexcept;

  std::intptr_t nativeHandle_ {-1};
  void* view_ {};
  std::size_t bytes_ {};
  bool alreadyExisted_ {};
};

} // namespace das::transport
