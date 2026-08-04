# 0.4.0-beta.2 一般配布チェックリスト

## 必須

- [x] 物理音声endpointへの自動フォールバックを廃止。
- [x] Discordの非公開event／DLL injectionへの依存を廃止。
- [x] AGPLv3／GPLv3全文をrepositoryとinstallerへ収録。
- [x] JUCE、VST3 SDK、r8brain、OBS、Inno Setupの第三者表示を収録。
- [x] 固定revisionを含む対応source archiveをinstallerへ同梱。
- [x] Installerが導入したfileだけをuninstallし、未知のdirectoryを再帰削除しない。
- [x] 使用中fileをreboot時に削除できるflagを設定。
- [x] Runtime telemetry／network通信なしを明文化。
- [x] Installerが仮想出力を読み取り専用で事前確認し、未検出時だけ公式導線を表示。
- [x] 同梱ソースだけを使うオフラインclean buildと全自動testを完走。
- [x] 配布者名を`SecondLunchi`に確定。
- [x] Copyright holder表記を`Copyright (c) 2026 SecondLunchi`に確定。
- [x] 問い合わせ先をGitHub Issuesに確定。
- [ ] Code-signing certificateのsubjectを確定し、binary／setup／uninstallerへ署名。
  初回ベータは未署名であることとSHA-256を明記して公開する。
- [x] Public source／Issues／ReleasesのURLを製品情報とREADMEへ設定。

## 実機試験

- [x] Windows 11／Studio One Pro 8でVST3を差し、ASIO出力を保ったままOBS録画を確認。
- [x] Discordの個人channelでDAW app shareの視聴側音声を確認。
- [x] Discordの個人channelで画面全体を直接shareし、VSTを含む映像と視聴側音声を確認。
- [ ] OBS＋Discord同時使用で二重音、drop、映像／音声の大きなずれがないことを確認。
- [ ] 1時間以上の連続試験。
- [ ] Clean install、0.3.0からupdate、uninstall、reboot後の残存確認。

## 公開物

- `DawAudioStreamer-Setup-0.4.0-beta.2.exe`
- `DawAudioStreamer-0.4.0-beta.2-source.zip`
- 両fileのSHA-256
- README、license、privacy、third-party notices
- 既知の制約と対象OBS／Windows／DAW環境
