# ビルド

[English](development.md)

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
- インストーラー：`build/installer/DawAudioStreamer-Setup-0.4.4.exe`

## Windows CIの成果物

WindowsワークフローはGitHubの実行環境でビルド・テストした後、未署名のインストーラーと対応ソースZIPを作成します。`DawAudioStreamer-Windows-unsigned-<commit>`にはSHA-256と、対象コミット・実行元を記録した`BUILD-INFO.json`も含まれます。インストーラーやソースZIPがなければ失敗します。保存期間は30日です。

PRのビルドはレビュー用です。リリース準備にはマージ後の`main`の成功ビルドを使用します。このワークフローは署名・タグ作成・リリース公開を行いません。署名は[コード署名方針](../CODE_SIGNING.ja.md)に記載した準備段階です。

パッケージ処理のテスト：`node --test tests/windows_package_tests.cjs`。一時フォルダ内のテストデータだけを使用し、実際のプラグインフォルダにはインストールしません。

## macOS（プレビュー版）

- IntelまたはApple Silicon搭載Mac・macOS 13以降
- Xcodeとコマンドラインツール（`xcode-select --install`）
- CMake 3.25以降
- Macのアーキテクチャに合ったOBS Studioを`/Applications/OBS.app`に

アーキテクチャに合わせてpresetを選びます。OBSはユニバーサルな`libobs`を配布していないため、インストール済みOBSと同じアーキテクチャしかビルドできません：

```zsh
cmake --preset macos-preview-intel   # または macos-preview-arm
cmake --build --preset macos-preview-intel
ctest --preset macos-preview-intel
```

配布用ZIPは`cmake/CreateMacPreviewPackage.cmake`で作成します（正確なコマンドは`macos-preview` CIワークフローを参照）。XcodeでSwift/AppKitのSetupアプリをビルドし、3種のプラグインを内包します。プラグインとアプリはアドホック署名であり、Developer ID署名・公証ではありません。CIは両アーキテクチャをネイティブでビルドし、一時ホームで導入・復旧・削除と、ZIPから展開した実payloadをテストします。

Macでの単体テスト：`swiftc -swift-version 5 installer/macos/SetupCore.swift tests/macos_setup_tests.swift -o /tmp/das-setup-tests` の後に `/tmp/das-setup-tests`。rootでは実行しないでください。実ユーザーのLibraryには導入しません。

新パッケージとサイトを同時公開する前に、ブラウザでダウンロードしたZIPを実機で確認してください。Gatekeeperの承認、日英の画面、DAWのプラグインスキャン、OBSソース追加、再導入・削除が対象です。Setupは隔離属性を除去せず、権限変更や管理者への昇格もしません。コピーの成功と、隔離されたプラグインをホストが読み込めることは別なので、公開前に必ず確認します。電源断・復旧失敗では復旧用フォルダを残して再変更を止めます。内容を確認するまで削除しないでください。

使用している依存ライブラリと固定revisionは
[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)に記載しています。Releaseの対応ソースZIPには、
オフラインで再ビルドできる依存ソースも含まれます。
