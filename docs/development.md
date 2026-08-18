# 開発環境

## 確認済み環境（2026-08-04）

- Windows x64
- Visual Studio Community 2026 18.7.x
- MSVC 14.51、Windows SDK 10.0.26100.0
- CMake／CTest
- Inno Setup 6.7.x
- OBS Studio 32.1.2 x64
- Discord desktop

カーネルドライバーは使用しないため、WDKは不要です。

## 固定依存revision

- JUCE 8.0.10：`3af3ce009f6a02f6fa651008fffb5b41743a9fab`
- r8brain-free-src：`8fff6f3db26f14a8f5e8fb871000613673db5753`
- OBS Studio 32.1.2：`c17423ce05899ecb93f678601b3feaa8a469b180`

CMake FetchContentはタグではなくcommit hashを使います。配布ソースアーカイブ内では
`third-party`を自動検出し、ネットワークなしで同じソースを使用します。

## ビルド・テスト・インストーラー

```powershell
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
ctest --preset windows-msvc-release
cmake --build build/windows-msvc-release --config Release --target das_installer
```

主な生成物：

- VST3：`build/windows-msvc-release/plugins/send-vst3/DasSend_artefacts/Release/VST3/DAS Send.vst3`
- OBS DLL：`build/windows-msvc-release/plugins/obs-source/Release/das-obs-source.dll`
- Installer確認helper：`build/windows-msvc-release/plugins/send-vst3/Release/das-virtual-audio-check.exe`
- DAS Engine：`build/windows-msvc-release/apps/engine/DawAudioStreamerEngine_artefacts/Release/DAS Engine.exe`
- 対応ソース：`build/source/DawAudioStreamer-0.4.0-beta.3-source.zip`
- Setup：`build/installer/DawAudioStreamer-Setup-0.4.0-beta.3.exe`

## 自動テスト

- `audio_core`：48 kHz bit-exact、44.1／96 kHz変換、20 kHz高域、alias除去、音声コア。
- `transport`：SPSCリング、overrun、stress。
- `transport_ipc`：別プロセス間の名前付き共有メモリ。
- `discord_bridge`：安全な初期値、対応仮想出力、物理出力へフォールバックしないこと。
- `vst3_smoke`：VST3実読込、複数インスタンス防止、SRC、OBSリング、
  Discord sessionが既知の仮想endpointだけに存在すること。

UI確認用の`das_vst3_editor_host`はWindows実画面で日本語、文字切れ、状態表示を確認します。

## リアルタイム規則

DAWの音声コールバックでは、メモリ確保、待機するlock、file I/O、logging、UI呼び出しを
行いません。SRCとscratch bufferはprepare時に確保します。共有リングが満杯でもproducerを
待たせません。WASAPIとadaptive drift correctionは専用worker threadで処理します。

## コード署名

一般公開時は、最終的なVST3 binary、OBS DLL、installer確認helperを署名し、その後に生成した
installerとuninstallerも同じ発行者で署名します。timestampを付け、署名後に改変しません。
証明書の秘密鍵、password、tokenはリポジトリやsource archiveへ含めません。

署名者名とpublisher名は`installer/DawAudioStreamer.iss`の`MyAppPublisher`およびbinary metadataで
一致させます。現在のベータ版は未署名であることとSHA-256を明記して公開します。安定版の
一般公開前には署名を必須チェックとして扱います。

## リリース前チェック

- 対象DAWでVST3検出、ASIO再生、bypass、project再読込。
- 44.1／48／96 kHzと代表的なASIO buffer size。
- OBS再起動、DAW停止／再開、plugin抜き差し、OBSとの同時利用。
- DiscordでDAW本体をアプリ共有し、視聴側の音声と二重音がないこと。
- Discordで画面全体を直接共有し、VSTを含む映像と視聴側音声を確認すること。
- 1時間以上の連続再生でbuffer correction、drop、音切れを確認。
- 新規install、上書き更新、使用中file、reboot後削除、uninstall後の残存物。
- Source ZIPだけを使ったclean offline build。
- Installerの仮想出力検出あり／なし／検出失敗ページとVB-CABLEリンク。
- SHA-256公開、binary／installerのcode signとtimestamp。
- 配布者名、copyright holder、問い合わせ先、source公開先を確認。

詳細な配布判定は[release-checklist.md](release-checklist.md)を参照してください。
