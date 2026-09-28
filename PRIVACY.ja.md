# プライバシー

[English](PRIVACY.md)

DawAudioStreamerは、テレメトリ、利用統計、クラッシュレポート、自動更新、広告、
アカウント機能を持ちません。音声処理のコンポーネントからネットワークへデータを送信しません。

Windows・macOSとも、同じユーザーのDAWとOBSソースの間で、共有メモリを使って音声を渡します。
WindowsではDiscordの画面共有向けにもローカルで音声を渡します。
DawAudioStreamerは音声ファイルの録音や画面映像の取得を行いません。録音・画面取得・配信はOBSやDiscordが行います。

Setupでヘルプのリンクを開くとブラウザーが起動します。サイトの閲覧・ファイルのダウンロードでは、
各配信サービスへの通信が発生します。サイトはGitHubからリリース情報を取得し、選んだ言語をブラウザー内に保存します。
これらはプラグイン内のローカルな音声処理とは別です。

各サービスへ送信した情報には、[GitHub](https://docs.github.com/en/site-policy/privacy-policies/github-general-privacy-statement)、
[OBS](https://obsproject.com/privacy-policy)、[Discord](https://discord.com/privacy)、利用する配信先のポリシーが適用されます。
DawAudioStreamerはDiscordのマイク設定、DAWのオーディオインターフェース設定、Windowsの既定音声デバイスを変更しません。
