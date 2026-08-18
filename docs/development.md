# ビルド

## 必要なもの

- Windows x64
- Visual Studio 2026（C++デスクトップ開発、Windows SDK）
- CMake 3.25以降
- Inno Setup 6（インストーラーを作る場合）

## 手順

```powershell
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
ctest --preset windows-msvc-release
```

インストーラーも作る場合：

```powershell
cmake --build build/windows-msvc-release --config Release --target das_installer
```

主な生成物：

- VST3：`build/windows-msvc-release/plugins/send-vst3/DasSend_artefacts/Release/VST3/DAS Send.vst3`
- OBSプラグイン：`build/windows-msvc-release/plugins/obs-source/Release/das-obs-source.dll`
- インストーラー：`build/installer/DawAudioStreamer-Setup-0.4.0-beta.3.exe`

使用している依存ライブラリと固定revisionは
[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)に記載しています。Releaseの対応ソースZIPには、
オフラインで再ビルドできる依存ソースも含まれます。
