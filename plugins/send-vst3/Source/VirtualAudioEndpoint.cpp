// SPDX-License-Identifier: AGPL-3.0-only
#include "VirtualAudioEndpoint.h"

#include <algorithm>
#include <cwctype>
#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Audioclient.h>
#include <propkeydef.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <Mmdeviceapi.h>
#include <Propsys.h>
#include <wrl/client.h>
#endif

namespace das::discord {

bool isSupportedVirtualAudioEndpointName(const std::wstring_view name) noexcept {
  std::wstring lower(name);
  std::transform(lower.begin(), lower.end(), lower.begin(), [](const wchar_t value) {
    return static_cast<wchar_t>(std::towlower(value));
  });
  return lower.find(L"elgato virtual audio") != std::wstring::npos ||
         lower.find(L"vb-audio virtual cable") != std::wstring::npos ||
         lower.find(L"cable input") != std::wstring::npos;
}

#if defined(_WIN32)
namespace {
using Microsoft::WRL::ComPtr;

std::wstring endpointName(IMMDevice* device) {
  if (device == nullptr) return {};
  ComPtr<IPropertyStore> properties;
  if (FAILED(device->OpenPropertyStore(STGM_READ, &properties))) return {};
  PROPVARIANT value;
  PropVariantInit(&value);
  const auto result = properties->GetValue(PKEY_Device_FriendlyName, &value);
  std::wstring name;
  if (SUCCEEDED(result) && value.vt == VT_LPWSTR && value.pwszVal != nullptr)
    name = value.pwszVal;
  PropVariantClear(&value);
  return name;
}
} // namespace

bool findSupportedVirtualAudioEndpoint(IMMDeviceEnumerator* enumerator,
                                       IMMDevice** device) noexcept {
  if (device != nullptr) *device = nullptr;
  if (enumerator == nullptr || device == nullptr) return false;
  ComPtr<IMMDeviceCollection> devices;
  if (FAILED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices))) return false;
  UINT count {};
  if (FAILED(devices->GetCount(&count))) return false;
  for (UINT index = 0; index < count; ++index) {
    ComPtr<IMMDevice> candidate;
    if (SUCCEEDED(devices->Item(index, &candidate)) &&
        isSupportedVirtualAudioEndpointName(endpointName(candidate.Get()))) {
      *device = candidate.Detach();
      return true;
    }
  }
  return false;
}
#endif

} // namespace das::discord
