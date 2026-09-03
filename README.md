# DawAudioStreamer

DAWの音を、ASIOのままOBSとDiscordへ。

DawAudioStreamerは、DAWのマスター音声を配信用にコピーするWindows向けVST3です。
DAWのオーディオ設定やDiscordのマイクを変更する必要はありません。

> 現在はベータ版です。配信前に短い録画や限定配信で動作を確認してください。

## 先に確認

- **OBSだけで使う：** DawAudioStreamerのインストーラーだけで使えます。
- **Discordにも音を載せる：** VB-CABLEが必要です。入っていないPCでは、インストール完了時に
  VB-CABLEの公式ページが開きます。

## 使い方

### 1. インストール

1. OBS、Discord、DAWを終了します。
2. [Releases](https://github.com/yoruhinot/DawAudioStreamer/releases)からインストーラーを入手します。
3. インストーラーを実行します。
4. DAWのマスターバスの最後へ「DAS Send」を1個挿します。

Discordでも使う場合は、開いた[VB-CABLE公式ページ](https://vb-audio.com/Cable/)から
ドライバーを導入し、案内に従ってWindowsを再起動してください。

### 2. OBS

1. OBSの「ソース」で［＋］を押します。
2. 「DAS Audio（DAW）」を追加します。
3. DAWを再生し、OBSの音声ミキサーが動けば完了です。

### 3. Discord

1. VSTに「Discord ● 直接共有用の音声を準備済み」と表示されていることを確認します。
2. Discordで［画面を共有］を開きます。
3. DAWだけを見せるならDAWアプリ、VST画面も見せるなら画面全体を選びます。

Discordのマイク設定はそのままで構いません。

## VSTの表示

| 表示 | 対処 |
|---|---|
| `OBS ● DAS Audioへ送信中` | OBSへ音声を送っています。 |
| `OBS ○ DAS Audioを待っています` | OBSで「DAS Audio（DAW）」を追加してください。 |
| `Discord ● 直接共有用の音声を準備済み` | Discordで共有を開始できます。 |
| `Discord × VB-CABLEを追加してください` | VB-CABLEを導入してWindowsを再起動してください。 |
| `△ DAS Sendが複数あります` | マスターバスに1個だけ残してください。 |

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

Studio One Pro 8、REAPER、Cubaseで動作を確認しています。

## アンインストール

DAWとOBSを終了し、Windowsの「インストールされているアプリ」からDawAudioStreamerを削除します。
DAWプロジェクト、OBSシーン、ASIO設定、別途導入した仮想オーディオドライバーは削除されません。

## ベータ版について

現在のインストーラーは未署名のため、Windowsが警告を表示する場合があります。ダウンロードは
[GitHub Releases](https://github.com/yoruhinot/DawAudioStreamer/releases)から行ってください。

不具合や要望は[Issues](https://github.com/yoruhinot/DawAudioStreamer/issues)へお願いします。

## ライセンス

ライセンスは[LICENSE](LICENSE)、第三者ソフトウェアの表示は
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)を参照してください。
[セキュリティとコード署名](CODE_SIGNING_POLICY.md)も確認できます。

ソースからビルドする場合は[ビルド手順](docs/development.md)を参照してください。
