# DawAudioStreamer 作業引き継ぎ

## 目標

オーディオインターフェースのASIOを使うDAWのマスター音声を、利用者がインストーラー実行後に
VST3「DAS Send」を1個挿すだけで、OBSとDiscord画面共有へ送れる状態にする。Discordはマイクを
置き換えず、DAWアプリ共有とVSTを含むモニター全体共有の両方に対応し、二重音を発生させない。

## 現在：0.4.0-beta.1 公開準備

- JUCE 8.0.10製DAS Send VST3。DAW音声は変更せずpass-through。
- r8brain 24-bit SRCで8～384 kHzから48 kHzへ変換。48 kHzはbit-exact。
- OBS native source「DAS Audio（DAW）」。
- DiscordはDAW processのWASAPI render sessionを使い、DAW app／monitor全体を直接shareできる。
- 物理endpointへのfallbackなし。Elgato Virtual Audio／VB-Audio Virtual Cable／CABLE Inputだけを使用。
- Installerは同じ判定ロジックで仮想出力を事前確認し、未検出時だけVB-CABLE公式導線を表示する。
- Discordの非公開event、DLL injection、microphone変更なし。
- ASIO／Windows clock差を40 ms targetのadaptive readerで補正。
- OBS／Discord／開発者用Engineの3本の独立SPSC ring（Engineはinstaller非同梱）。
- VSTの日本語UIをWindows実画面で確認済み。
- 自動test 5/5 pass。対応sourceだけを使うoffline clean buildもpass。
- AGPL／GPL全文、third-party notices、privacy、対応source package targetを追加済み。
- Publisher／copyright holderは`SecondLunchi`。公開先は
  `https://github.com/SecondLunchi/DawAudioStreamer`、問い合わせはGitHub Issues。
- Installerへsource archive、安全なuninstall、公開URLを追加済み。
- 公開artifact名は`build/installer/DawAudioStreamer-Setup-0.4.0-beta.1.exe`、対応sourceは
  `build/source/DawAudioStreamer-0.4.0-beta.1-source.zip`。両artifactを生成済みで、自動testは5/5 pass。
  Setup／binaryは初回ベータでは未署名。

## 重要な設計判断

WASAPI sessionをmuteするとMicrosoft ApplicationLoopbackのprocess captureでも無音になったため、
physical endpointへrenderしてsessionだけmuteする方法は不可。Discord音声はknown silent virtual
endpointがある時だけ動かす。対応endpointがなければVSTに×を出し、OBSだけ使える。

Windows版Discordの実機確認で、同じDAW側sessionはDAW app shareだけでなくmonitor全体shareでも
視聴側へ届いた。VST側のmode選択や映像中継アプリは設けず、Discordで共有範囲を直接選ぶ。

## 次の作業

1. GitHubへ変更をpushし、内容を確認してmainへ反映する。
2. GitHub Releaseへinstaller／source ZIP／SHA-256／release notesを掲載する。
3. Install／upgrade／uninstall／reboot cleanupを追加で実機testする。
4. 安定版までにcode signing certificateを用意し、binary／setup／uninstallerへ署名する。

## まだ実機確認が必要

- OBS／Discord同時利用での二重音、drop、映像／音声同期の長時間確認。
- 1時間以上のclock drift試験。
- 0.3.0からの上書き更新と完全uninstall。
- Code signing。

手順と現在の公開blockerは`README.md`と`docs/release-checklist.md`を参照。
