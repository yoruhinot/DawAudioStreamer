# Build

[日本語](development.ja.md)

## Requirements

- Windows x64
- Visual Studio 2026 (Desktop development with C++, Windows SDK)
- CMake 3.25 or later
- Inno Setup 6 (only needed to build the installer)

## Steps

```powershell
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
ctest --preset windows-msvc-release
```

To also build the installer:

```powershell
cmake --build build/windows-msvc-release --config Release --target das_installer
```

Key build outputs:

- VST3: `build/windows-msvc-release/plugins/send-vst3/DasSend_artefacts/Release/VST3/DAS Send.vst3`
- OBS plugin: `build/windows-msvc-release/plugins/obs-source/Release/das-obs-source.dll`
- Installer: `build/installer/DawAudioStreamer-Setup-0.4.4.exe`

## Windows CI artifacts

The Windows workflow builds and tests on GitHub-hosted runners, then creates the unsigned installer and matching source ZIP. Its `DawAudioStreamer-Windows-unsigned-<commit>` artifact also includes SHA-256 hashes and `BUILD-INFO.json` with the exact checkout and workflow run. A missing installer or source ZIP fails the build. Artifacts are kept for 30 days.

PR builds are for review only. After merging, use a successful `main` build for release preparation; the workflow does not sign files, create tags, or publish releases. Signing remains pending under the [code signing policy](../CODE_SIGNING.md).

Packaging tests run without installing software: `node --test tests/windows_package_tests.cjs`. Their fixtures live in temporary directories, not your plug-in folders.

## macOS (preview)

- Intel or Apple Silicon Mac, macOS 13 or later
- Xcode with command line tools (`xcode-select --install`)
- CMake 3.25 or later
- OBS Studio (matching your Mac's arch) at `/Applications/OBS.app`

Pick the preset for your arch — you can only build the arch of your installed OBS, since OBS ships no universal `libobs`:

```zsh
cmake --preset macos-preview-intel   # or macos-preview-arm
cmake --build --preset macos-preview-intel
ctest --preset macos-preview-intel
```

The distributable ZIP is built by `cmake/CreateMacPreviewPackage.cmake` (see the `macos-preview` CI workflow for the exact invocation). It compiles the Swift/AppKit Setup app with the native Xcode toolchain and embeds the three plugins. Bundles and the outer app are ad-hoc signed, not Developer ID signed or notarized. CI builds both arches on native runners and tests installation, rollback and removal in temporary homes, including the extracted ZIP's actual payload.

Run the isolated installer tests on macOS with `swiftc -swift-version 5 installer/macos/SetupCore.swift tests/macos_setup_tests.swift -o /tmp/das-setup-tests` then `/tmp/das-setup-tests`. Do not run as root. The tests never install into your real Library.

Before publishing the new package and site together, test a browser-downloaded ZIP on a Mac: Gatekeeper approval, Japanese/English setup windows, plugin scanning in a DAW, OBS source creation, reinstall and uninstall. File copying preserves quarantine; Setup never removes it, changes permissions, or elevates privileges. A successful file installation does not prove the host will allow a quarantined plugin to load. Test that boundary explicitly before release. Power loss or failed rollback leaves labeled recovery folders and blocks further changes; do not delete recovery files before inspecting them.

Dependencies and their pinned revisions are listed in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). The release source ZIP includes offline-rebuildable dependency sources.
