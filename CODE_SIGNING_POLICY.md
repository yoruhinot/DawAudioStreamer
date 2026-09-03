# Code signing policy

DawAudioStreamerは、Windows正式版へのコード署名を準備しています。
現在公開中のベータ版は未署名です。署名の状態は各リリースページに明記します。

## Windows release signing

SignPath Foundationのオープンソース向け署名サービスへ申請予定です。承認後は、
公開リポジトリから自動生成された正式リリースだけを署名対象にします。

Free code signing provided by [SignPath.io](https://signpath.io/), certificate by
[SignPath Foundation](https://signpath.org/).

### Team roles

- Committer and reviewer: [yoruhinot](https://github.com/yoruhinot)
- Release approver: [yoruhinot](https://github.com/yoruhinot)

## Privacy

本ソフトウェアは、利用者またはインストール・操作を行う人が明示的に要求しない限り、
ネットワーク上の他のシステムへ情報を送信しません。

## System changes

WindowsインストーラーはDAS Send VST3、OBS用音声ソース、アンインストーラー、
ライセンス文書を追加します。DAWのASIO設定、既定の音声デバイス、Discordのマイク設定は
変更しません。VB-CABLEは同梱せず、利用者が公式配布元から別途導入します。

Windowsの「インストールされているアプリ」からDawAudioStreamerを削除できます。
