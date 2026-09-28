# Code signing policy

[English](CODE_SIGNING.md)

## 現在の状態

Windows版は現在、未署名です。[SignPath Foundation](https://signpath.org/)への再申請を準備しています。採択・署名の提供はまだ受けていません。

macOS版はアドホック署名で、Apple Developer IDによる署名・公証は行っていません。今回も変更しません。

## Windows版の署名方針

- 対象はDawAudioStreamerのインストーラーと同梱する自作プラグインです。別途配布されるVB-CABLEなどは含みません。
- 公開リポジトリのソースからGitHubの実行環境で作成したファイルだけを署名に使用します。PRのビルドは検証用で、リリース署名には使用しません。
- 開発・レビュー・署名承認は[夜人 / yoruhinot](https://github.com/yoruhinot)が担当します。外部からのPRはマージ前にレビューし、正式な署名は毎回、管理者が明示的に承認します。
- 署名の開始前に、署名アカウントとリポジトリへのアクセスに多要素認証を設定し、対象ブランチ・製品情報・署名するファイルをSignPath上で制限・確認します。
- 採択後の署名名義は活動名ではなく**SignPath Foundation**になります。正式な提供元表記はサービスの利用開始時に追加します。現在の支援を示すものではありません。

公式配布先：[GitHub Releases](https://github.com/yoruhinot/DawAudioStreamer/releases)。CIの成果物は未署名の検証用ビルドで、公開リリースではありません。

[プライバシーポリシー](PRIVACY.ja.md)
