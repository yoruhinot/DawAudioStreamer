# ADR 0002: Discord共有はDAWプロセスの無音仮想出力へ直接送る

- 状態：採用（2026-08-04に実機結果を反映）
- 初版：2026-08-02

## 背景

必要なのはDiscordのマイク入力を置き換えることではなく、オーディオインターフェースのASIOを
継続使用するDAWのマスター音声を画面共有へ載せることです。DAW本体のアプリ共有に加え、VST画面を
含むモニター全体の共有が必要です。ASIOの本来のモニター音とWindows renderへ複製した音が同じ
物理出力から聞こえると二重音になります。

## 決定

DAWアプリ共有では、DAS SendがDAWプロセス内に通常のWASAPI render sessionを作ります。
render先は既知の無音仮想出力だけとし、見つからなければDiscord経路を停止します。物理endpointへの
フォールバックは行いません。

Windows版Discordの実機確認で、同じDAW側sessionはDAWアプリ共有だけでなく画面全体共有でも
取得できました。利用者はDiscordの公開UIでDAWアプリまたは対象モニターを直接選びます。

共有開始／停止の検出にはDiscordの非公開イベントを使いません。VSTの表示は、DAW側sessionの
準備状態だけに限定します。本製品は画面映像を取得しません。

## 理由

- DAWのASIO、オーディオインターフェース、Discordのマイクを変更しない。
- 物理出力へ配信用コピーを流さず、二重音を設計上防ぐ。
- VSTを含む画面全体でも、Discordの公開UIだけで完結する。
- Discordの非公開イベント名やDLL注入へ依存しない。
- OBS経路と分離でき、同時利用できる。
- カーネルドライバーを自作・同梱せず、ドライバー署名やテストモードを不要にする。

## 検証

- Microsoft ApplicationLoopbackサンプルで、DAW相当テストホストのWASAPI sessionから
  440 Hz信号を取得できることを確認した。
- 同じsessionをWindows側でmuteすると、プロセスloopback側もほぼ無音になることを確認した。
  そのため「物理endpoint＋session mute」は採用できない。
- Elgato Virtual Audio上のsessionでは物理スピーカーへ重複再生せず、プロセスloopbackで取得できた。
- Windows版Discordで画面全体を直接共有し、DAW側sessionの音声が視聴側へ届くことを確認した。

## 影響と制約

- DiscordにはElgato Virtual Audio、VB-Audio Virtual Cable、CABLE Inputのいずれかが必要。
- 第三者仮想ドライバーはライセンスと署名の責任範囲を分けるため同梱しない。
- 画面全体の音声取得挙動はDiscord更新の影響を受け得るため、対象Discord版でリリース前実機試験を行う。
- 画面全体共有でほかのWindowsアプリ音が混ざる環境では、DAWアプリ共有を使用する。
- 将来仮想マイクが必要になった場合は別機能とし、この画面共有経路へ混在させない。
