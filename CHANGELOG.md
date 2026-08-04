# 変更履歴

このプロジェクトの利用者向け変更を記録します。

## [0.4.0-beta.1] - 2026-08-04

初回公開ベータ版です。

### 追加

- DAWのマスター音声を変更せず複製するVST3「DAS Send」
- OBSの専用音声ソース「DAS Audio（DAW）」
- DiscordのDAWアプリ共有／画面全体共有へ音声を渡すWindows音声セッション
- 8～384 kHzから48 kHzへの高品質サンプルレート変換とクロック差補正
- VST画面の日本語ステータス表示
- 対応する無音仮想オーディオ出力のインストール前確認
- ライセンス、第三者表示、対応ソースZIP、安全なアンインストーラー

### 確認済み

- Windows 11とStudio One Pro 8でのVST3読み込み
- OBS録画へのDAW音声入力
- Discord画面共有の視聴側音声

### 既知事項

- Windows x64専用です。
- Discord利用時は対応する無音仮想出力が必要です。何も入っていないPCではVB-CABLEを別途導入します。
- インストーラーとバイナリは未署名のため、Windowsが警告を表示する場合があります。
- ベータ版のため、長時間配信や未確認のDAW／オーディオ環境では事前テストを推奨します。

[0.4.0-beta.1]: https://github.com/SecondLunchi/DawAudioStreamer/releases/tag/v0.4.0-beta.1
