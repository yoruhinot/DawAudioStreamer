# アーキテクチャ

## 全体経路

```text
DAW audio callback（オーディオインターフェースのASIO）
  ├─→ DAW本来の出力 → オーディオインターフェース → 利用者のモニター
  └─→ DAS Send VST3 → 48 kHz／32-bit float／stereo
       ├─→ OBSリング → OBS「DAS Audio（DAW）」→ 配信／録画
       ├─→ Discordリング → DAWプロセスのWASAPI render → 無音仮想出力
       └─→ Engineリング → DAS Engine（任意の確認用モニター）
```

DAS Sendは入力バッファを書き換えず、DAWへ同じ音声を返します。ASIOデバイス、DAWのモニター、
Discordのマイク、Windowsの既定音声デバイスは変更しません。OBS、Discord、Engineは
独立したSPSCリングを使うため、同時に動かしても互いの読み取り位置を奪いません。

## DAS Send VST3

- マスターバスの音声を変更せずパススルーする。
- 48 kHz以外ではr8brain `CDSPResampler24`を使い、8～384 kHzを48 kHzへ変換する。
- 48 kHz入力はbit-exactに配信バッファへコピーする。
- prepare時に全バッファと変換器を確保し、音声コールバックでは確保・ロック・I/O・UIを行わない。
- 名前付きイベントで送信インスタンスを1個に限定し、複数挿入時の二重送信を止める。
- consumer heartbeatを読み、OBSの接続状態を日本語UIへ表示する。

## サンプルレート変換

r8brainの24-bit向け線形位相変換器を、3.5%の遷移帯域で使用します。44.1 kHzの20 kHz信号を
保持しながら、96→48 kHzで出力ナイキストを超える入力を十分に減衰させます。フィルターの
初期遅延は配信コピー側だけにあり、DAWからオーディオインターフェースへ返す音声には加えません。

## OBS音声ソース

`DAS Audio（DAW）`はOBS Studio内で動くネイティブ入力ソースです。OBS専用リングから
48 kHz／stereo／floatを読み、OBSのplanar float音声APIへ渡します。Windowsのデスクトップ音声、
DAS Engine、Discord経路には依存しません。起動時は蓄積した古い音声を捨て、ライブ位置から始めます。

## Discord：直接共有

DAS Sendと同じDAWプロセスに通常のWASAPI render sessionを作ります。DiscordではDAW本体の
「アプリ」または「画面全体」のどちらを共有しても、このsessionを画面共有音声として取得できます。
映像はDiscord自身が取得し、本製品は画面を読み取りません。

render先は次の既知の無音仮想出力だけです。

- Elgato Virtual Audio
- VB-Audio Virtual Cable
- CABLE Input

対応endpointがなければ`virtualOutputRequired`となり、物理出力へフォールバックしません。
WASAPI sessionをミュートするとWindowsのプロセスループバックでも無音になることを実測したため、
「物理出力へ流してsessionだけミュートする」方式は採用していません。

セットアップはVSTと同じ`findSupportedVirtualAudioEndpoint`を使う一時helperを実行し、導入前に
対応endpointの有無を表示します。helperはendpointを列挙するだけで、ドライバー、音量、既定設定を
変更しません。未検出時もOBS用途のためインストールは続行できます。

Discordの共有状態を示す非公開イベントや、DiscordプロセスへのDLL注入は使用しません。VSTは
「共有中」と推測せず、音声sessionの準備状態だけを正直に表示します。

DAWだけを見せるか、VSTを含むモニター全体を見せるかはDiscordの公開UIで選びます。画面全体共有で
ほかのWindowsアプリ音まで混ざる環境では、DAWアプリ共有へ戻して対象を絞ります。VST側に共有モードを
設けず、映像中継用の別アプリも起動しません。

## 長時間のクロック差

ASIO側とWindows endpoint側は、どちらも48 kHz表記でも物理クロックがわずかに異なります。
DiscordBridgeは約40 msを目標にした8,192-frame FIFOを持ち、Catmull-Rom補間で消費率を
0.997～1.003倍の範囲に微調整します。FIFO残量が目標へ戻る方向に制御するため、長時間配信での
周期的なunderflow／overflowを抑えます。大きな停止・復帰時だけ再バッファします。

## DAS Engine（開発者向け）

確認用モニターです。Engine専用リングを読み、利用者が選んだWindows再生デバイスへ出力します。
通常のOBS／Discord配信には不要で、二重音の原因になり得る余分な操作をなくすため、利用者向け
インストーラーには含めません。開発時の確認だけに使用します。

## 転送仕様

- 形式：32-bit float、interleaved stereo、48 kHz
- リング容量：96,000 frames（2秒）× consumer別
- 同期：64-bit lock-free atomicによるSPSC
- 互換性：magic、protocol major/minor、channels、sample rateを検証
- 過負荷：producerを待たせず、収まらないframeをdrop countへ記録
- 状態：producer／consumer heartbeat

採用判断は[ADR 0002](adr/0002-discord-screen-share-bridge.md)に記録しています。
