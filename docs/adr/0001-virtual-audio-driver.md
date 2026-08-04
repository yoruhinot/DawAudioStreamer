# ADR 0001：SysVAD派生の仮想オーディオドライバーを採用する（廃止）

- 状態：廃止（ADR 0002で置き換え）
- 日付：2026-08-02

## 当初の決定

DawAudioStreamerには、MicrosoftのSysVADサンプルを基にした専用の仮想音声ドライバーを
オプションとして同梱します。配布パッケージはDawAudioStreamer名義のWaveRTキャプチャ
エンドポイントを提供し、VB-CABLE、VoiceMeeter、JACK、Virtual Audio Cableを必須にしません。
ユーザーモードEngineでは、既存の物理／仮想WASAPI・ASIOデバイスも選択可能にします。

`VirtualDrivers/Virtual-Audio-Driver` は実装の参考にできますが、バイナリ依存にはしません。
ソースを再利用する場合はMIT表示を維持し、SysVAD派生部分にはMicrosoft Public Licenseの
表示を保持します。

## 廃止理由

利用要件が「Discordのマイクを置き換える」ことではなく、「DAW本体の画面共有へ音を載せる」
ことだと確定しました。この用途はDAWプロセス内のWASAPI render streamで実現でき、
カーネルドライバー、テスト署名、一般配布用ドライバー署名を不要にできます。現行方式は
[`0002-discord-screen-share-bridge.md`](0002-discord-screen-share-bridge.md)を参照してください。

## 理由

- 独自ライセンスの仮想ケーブルは、OSSだけで完結する配布要件を満たさない。
- JACKやScreamは隣接問題を解決するが、Discord／OBS向けのワンクリック体験には適さない。
- SysVADはMicrosoftが提供する仮想WDM音声の参照実装である。
- 転送層を限定して所有すれば、DSPをカーネル外に保ちながら一貫した製品にできる。

## 影響

- 開発版にはWindowsのテスト署名設定が必要。
- 一般配布にはMicrosoftに受理された署名済みドライバーパッケージが必要。
- ドライバーソース、第三者表示、再現可能なビルド手順、正確な上流リビジョンを同梱する。
- ユーザー／カーネル境界を越えるプロトコルをバージョン管理し、Fuzz Testを行う。
- インストーラーにはドライバー導入失敗時のロールバックと安全なアンインストールが必要。
