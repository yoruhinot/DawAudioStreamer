# DawAudioStreamer

[English](README.md)

DAWの音を、ASIOのままOBSとDiscordへ。

DawAudioStreamerは、DAWのマスター音声をOBSなどへ送る配信用プラグインです。
Windows版（VST3）とIntel／Apple Silicon向けmacOSプレビュー版（AU／VST3）があります。

> macOS版はプレビュー版です。配信前に短い録画や限定配信で動作を確認してください。

## 先に確認

- **OBSだけで使う：** DawAudioStreamerのインストーラーだけで使えます。
- **WindowsでDiscordにも音を載せる：** VB-CABLEが必要です。[Discordの導入手順](https://yoruhinot.github.io/DawAudioStreamer/?lang=ja#discord)へ進んでください。

macOSでは主にOBS向けに使用します。DiscordはmacOS標準の画面共有音声を利用できるため、
DawAudioStreamerや仮想オーディオデバイスは不要です。

## 使い方

### 1. インストール

1. OBS、Discord、DAWを終了します。
2. [Releases](https://github.com/yoruhinot/DawAudioStreamer/releases)からインストーラーを入手します。
3. Windowsはインストーラーを実行。MacはZIPを展開し、**DawAudioStreamer Setup.app**を開き、インストール完了を確認します。
4. DAWのマスターバスの最後へ「DAS Send」を1個挿します。

WindowsでDiscordも使う場合は、[VB-CABLEの導入手順](https://yoruhinot.github.io/DawAudioStreamer/?lang=ja#vbcable)に沿って導入し、Windowsを再起動してください。OBSには不要です。

Macで起動をブロックされた場合は[Macの導入ヘルプ](https://yoruhinot.github.io/DawAudioStreamer/?lang=ja#macos-help)へ。旧版のMac ZIPはInstall.commandを使用します。

### 2. OBS

1. OBSの「ソース」で［＋］を押します。
2. 「DAS Audio（DAW）」を追加します（設定不要）。旧版で「有効なプロパティがありません」と出ても正常です。
3. DAWを再生し、OBSの音声ミキサーや録画などで音を確認します。

### 3. Discord

1. DAS SendのDiscord欄が緑の `OK` になっていることを確認します。
2. Discordで［画面を共有］を開きます。
3. DAWだけを見せるならDAWアプリ、VST画面も見せるなら画面全体を選びます。

Discordのマイク設定はそのままで構いません。
視聴する人にも音が届くことを確認してください。

## VSTの表示

| 表示 | 対処 |
|---|---|
| 緑の `OK` | OBSは接続、Discordは共有用音声の準備を示します。録画・視聴側でも音を確認してください。 |
| 灰色の `WAIT` | OBSで「DAS Audio（DAW）」を追加してください。 |
| 黄色の `VB-CABLE` | VB-CABLEを導入してWindowsを再起動してください。 |
| 黄色の `1 ONLY` | DAS Sendを1個だけ残してください。 |
| 赤い `!` | OBS、DAWの順に起動し直してください。 |
| 灰色・文字なし | DAS Sendがバイパスされています。 |

## 音が二重に聞こえる場合

- DAS Sendはマスターバスに1個だけ挿します。
- OBSでは「DAS Audio（DAW）」と同じ音をデスクトップ音声などから同時に取り込まないでください。
- DiscordではDAWアプリ共有と画面全体共有を同時に開始しないでください。

## 音質

DAS SendはDAWへ戻す音声や音量を変更しません。配信用の音声はステレオ48 kHzへ変換され、
最後にOBSやDiscord側の設定で圧縮されます。音割れを防ぐため、DAWのマスターは0 dBFS未満にしてください。

## 対応環境

- Windows 11 x64
- VST3対応の64-bit DAW
- OBS Studio x64
- Discordデスクトップ版
- IntelまたはApple Silicon搭載Mac・macOS 13以降（AU／VST3、プレビュー版）

Fender Studio Pro 8、REAPER、Cubase、Ableton Live 12で動作を確認しています。

## アンインストール

DAWとOBSを終了し、Windowsの「インストールされているアプリ」からDawAudioStreamerを削除します。
DAWプロジェクト、OBSシーン、ASIO設定、別途導入した仮想オーディオドライバーは削除されません。

## ダウンロードと問い合わせ

ダウンロードは[GitHub Releases](https://github.com/yoruhinot/DawAudioStreamer/releases)から行ってください。
インストール時にWindowsまたはmacOSの警告が表示される場合があります。

不具合や要望は[X（@yoruhinot）](https://x.com/yoruhinot)または[Issues](https://github.com/yoruhinot/DawAudioStreamer/issues)へお願いします。

## ライセンス

ライセンスは[LICENSE](LICENSE)、第三者ソフトウェアの表示は
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)を参照してください。

ソースからビルドする場合は[ビルド手順](docs/development.ja.md)を参照してください。
