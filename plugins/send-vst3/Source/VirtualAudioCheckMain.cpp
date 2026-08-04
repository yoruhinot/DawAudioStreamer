// SPDX-License-Identifier: AGPL-3.0-only
#include "VirtualAudioEndpoint.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Mmdeviceapi.h>
#include <wrl/client.h>

int wmain() {
  const auto comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  const auto uninitialize = SUCCEEDED(comResult);
  if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) return 2;

  Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
  const auto createResult = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                             IID_PPV_ARGS(&enumerator));
  if (FAILED(createResult)) {
    if (uninitialize) CoUninitialize();
    return 2;
  }

  Microsoft::WRL::ComPtr<IMMDevice> endpoint;
  const auto found = das::discord::findSupportedVirtualAudioEndpoint(
      enumerator.Get(), endpoint.GetAddressOf());
  if (uninitialize) CoUninitialize();
  return found ? 0 : 1;
}
#else
int main() { return 2; }
#endif
