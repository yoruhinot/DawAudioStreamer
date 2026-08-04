// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <string_view>

#if defined(_WIN32)
struct IMMDevice;
struct IMMDeviceEnumerator;
#endif

namespace das::discord {

[[nodiscard]] bool isSupportedVirtualAudioEndpointName(std::wstring_view name) noexcept;

#if defined(_WIN32)
[[nodiscard]] bool findSupportedVirtualAudioEndpoint(IMMDeviceEnumerator* enumerator,
                                                     IMMDevice** device) noexcept;
#endif

} // namespace das::discord
