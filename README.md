# DawAudioStreamer

DawAudioStreamerは、オーディオインターフェースのASIOを使っているDAWのマスター音声を、
DAW側の音声設定を変えずにOBSとDiscordの画面共有へ送るWindows x64用ツールです。

> 現在は`0.4.0-beta.1`です。実際の配信前に短い録画または限定公開で音量と同期を確認してください。

- DAWのマスターにVST3「DAS Send」を1個挿すだけで音声を複製します。
- OBSには専用音声ソース「DAS Audio（DAW）」として届きます。
- Discordではマイクを変更せず、DAWアプリ共有と画面全体共有のどちらにも音声を載せます。
- VST画面も見せたい場合は、Discordで画面全体を直接選ぶだけです。
- ASIOの本来のモニター音はそのままです。配信用コピーを物理出力へ流さないため、二重音を防ぎます。

## 対応環境

- Windows 11 x64
- VST3対応の64-bit DAW（Studio One Pro 8で実機確認済み）
- OBS Studio x64
- Discordデスクトップ版
- Discord利用時のみ、対応する無音仮想オーディオ出力

## 最短セットアップ

1. OBS、Discord、DAWを終了します。
2. [GitHub Releases](https://github.com/SecondLunchi/DawAudioStreamer/releases)から
   `DawAudioStreamer-Setup-0.4.0-beta.1.exe`を入手して実行します。
3. DAWを起動し、マスターバスの最後へ「DAS Send」を1個挿します。
4. DAS Sendの画面でOBS／Discordの状態を確認します。

セットアップは対応する無音仮想出力を事前確認します。「Elgato Virtual Audio」がすでに入っている
PCでは、Discord用の追加設定は不要です。それ以外のPCでは、Elgato Virtual Audio、VB-Audio
Virtual Cable、CABLE Inputのいずれかが必要です。これらの第三者ドライバーは本インストーラーに
同梱せず、既定デバイスにも設定しません。仮想出力がない場合もOBSは使えますが、DiscordはVSTに
「× 無音の仮想出力が必要」と表示して安全に停止します。

未検出時はセットアップ完了画面から[VB-CABLE公式ページ](https://vb-audio.com/Cable/)を開けます。
VB-CABLEの導入はVB-Audio自身のセットアップで行い、表示された再起動やWindowsの既定音声設定を
確認してください。DawAudioStreamerは第三者ドライバーを自動導入・削除しません。

## OBSで使う

1. OBSの「ソース」欄で［＋］を押します。
2. 「DAS Audio（DAW）」を追加します。
3. DAWを再生し、OBSの音声ミキサーが動けば完了です。

DAS Sendが「OBS ● DAS Audioへ送信中」になれば正常です。OBSの「デスクトップ音声」や
別のループバック機能でも同じDAW音声を取っている場合は、どちらか一方をミュートしてください。

## Discordで使う

1. DAS Sendが「Discord ● 直接共有用の音声を準備済み」になっていることを確認します。
2. Discordで［画面を共有］を開きます。
3. DAWだけを見せるなら［アプリ］からDAW本体を選びます。
4. VST画面やほかのウィンドウも見せるなら［画面］から対象モニターを選びます。
5. 共有を開始します。

DAS Send側のモード選択や追加アプリはありません。Discordのマイク設定は変更せず、DAWは普段どおり
オーディオインターフェースのASIOを使い続けます。画面全体共有でほかのWindowsアプリの音まで
混ざる環境では、DAW本体のアプリ共有を選ぶと対象を絞れます。

## VSTの表示

- `OBS ● DAS Audioへ送信中`：OBSの専用ソースが受信中です。
- `OBS ○ DAS Audioを待っています`：OBSが停止中、またはソースが未追加です。
- `Discord ● 直接共有用の音声を準備済み`：DAWアプリまたは画面全体を共有できます。
- `Discord × 無音の仮想出力が必要です`：物理出力へ迂回せず、安全のためDiscord出力を停止しています。
- `△ DAS Sendが複数あります`：マスターに1個だけ残してください。

## 音質と遅延

- DAWから配信経路までは32-bit float、ステレオです。
- 8～384 kHzのDAW音声を、r8brainの24-bit向けバンド制限SRCで48 kHzへ変換します。
- 48 kHzではサンプルを変更せず、そのまま通します。
- 44.1→48 kHz、96→48 kHz、20 kHzの可聴高域保持、ナイキスト超成分の除去を自動テストしています。
- Windows側とのクロック差は約40 msの適応バッファで吸収し、長時間配信の音切れを抑えます。

OBSやDiscordでは最後に各サービスの配信コーデックで圧縮されます。DAS Sendは音量を上げないので、
音割れを避けるにはDAWのマスターを0 dBFS未満に保ってください。DAS Send自身はDAWへ返す音声を
変更しません。

## 二重音を防ぐチェック

- DAS Sendはマスターバスに1個だけ挿します。
- OBSでは「DAS Audio（DAW）」と同じDAW音を別ソースから同時取得しません。
- Discordでは共有を1つだけ開始します。DAWだけならアプリ共有、VSTも映すなら画面全体共有です。
- 無音仮想出力をWindowsの既定出力に変更する必要はありません。

## アンインストール

1. DAWとOBSを終了します。
2. Windowsの「インストールされているアプリ」からDawAudioStreamerをアンインストールします。
3. 使用中だったファイルの削除を求められた場合だけ再起動します。

アンインストーラーは、セットアップが導入したVST3、OBSプラグイン、文書、ライセンス、
対応ソース、レジストリ値、スタートメニュー項目を削除します。DAWプロジェクト、OBSシーン、
オーディオインターフェースのASIO設定、ElgatoやVB-Audioなどの第三者ドライバーは削除しません。
インストール先へ利用者が追加した未知のファイルは再帰削除しないため、必要なら後から手動で確認できます。

## 配布・ライセンス

この無料配布版は、JUCEをリンクする部分を含めてAGPL-3.0-only条件で配布します。インストーラーには
ライセンス全文、第三者表示、バイナリに対応する本プロジェクトと固定revisionの依存ソースを含む
`DawAudioStreamer-0.4.0-beta.1-source.zip`を同梱します。詳細は[LICENSE](LICENSE)と
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)を参照してください。

本ソフトウェアはOBS Project、Discord Inc.、Steinberg Media Technologies GmbH、Elgato、
VB-Audioの公式製品ではなく、各社からの承認・提携を意味しません。VSTはSteinberg Media
Technologies GmbHの商標です。各製品名・商標は各権利者に帰属します。

このベータ版はコード署名されていないため、Windowsが発行元不明またはSmartScreenの警告を表示する
場合があります。Releaseに掲載されたSHA-256と一致しないファイルは実行しないでください。将来の
一般安定版では、配布者を確認できるコード署名を行う予定です。

## 不具合報告

不具合や使いにくい点は[GitHub Issues](https://github.com/SecondLunchi/DawAudioStreamer/issues)へ
報告してください。Windows／DAW／オーディオインターフェース／ASIOドライバー／サンプルレート／
バッファサイズ／OBS／Discordの各バージョンがあると原因を絞りやすくなります。公開Issueへ、DAWの
プロジェクトファイル、アカウント情報、配信URLなどの個人情報は添付しないでください。

## 開発

必要な環境と手順は[docs/development.md](docs/development.md)、構成は
[docs/architecture.md](docs/architecture.md)を参照してください。

```powershell
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
ctest --preset windows-msvc-release
cmake --build build/windows-msvc-release --config Release --target das_installer
```
