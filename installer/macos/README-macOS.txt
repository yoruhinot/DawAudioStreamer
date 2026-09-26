DawAudioStreamer — macOS
=======================
macOS 13+ · Intel or Apple Silicon (download the ZIP for your Mac)

1. Extract the entire ZIP and close your DAW and OBS.
2. Open DawAudioStreamer Setup.app and click Install / Update.
3. Wait for “Installation complete”, then reopen your DAW and OBS.
4. Insert one DAS Send at the end of your DAW master.
5. In OBS Sources, add DAS Audio (DAW). No source settings are needed.
6. Play audio, check the OBS meter and listen to a short recording.

Setup follows your Mac's language; 日本語 / English switches it.
Keep Setup.app to update or uninstall later. No Terminal commands are needed.

If macOS blocks opening
-----------------------
After trying to open Setup, go to System Settings > Privacy & Security >
Open Anyway, then confirm Open. Only do this for your trusted official download.
Do not bypass a malware or damaged-app warning; stop and contact support.
Managed Macs may need their administrator's approval.
https://support.apple.com/102445

If Setup does not complete
-------------------------
Do not continue to the DAW steps until Setup says “Installation complete”.
- Cannot write: Show in Finder, then Get Info > Sharing & Permissions. If your
  account cannot write, ask the Mac's administrator to check that folder.
  Setup never changes permissions or requests administrator access itself.
- Missing/unverifiable files: download the official ZIP again and extract it fully.
- Recovery files: do not delete them. Copy details and send them to support.
- Cleanup warning: plugins were installed/removed, but temporary files remain.
  Send the warning details to support before running Setup again.

Installed for this user only
---------------------------
~/Library/Audio/Plug-Ins/VST3/DAS Send.vst3
~/Library/Audio/Plug-Ins/Components/DAS Send.component
~/Library/Application Support/obs-studio/plugins/das-obs-source.plugin

If DAS Send is missing, reopen the DAW and rescan VST3/AU plugins. If macOS
blocks a plugin, check Privacy & Security and the official guide; do not disable
security features. If DAS Audio is missing, restart OBS and check for OBS updates.
Older DAS versions may show “No properties available” in OBS; this is normal.

Discord: macOS normally shares DAW audio without DAS. Share your DAW application
or screen with audio enabled, then ask a viewer to check the sound.

Uninstall: close your DAW and OBS, open Setup.app and choose Uninstall.
Wait for “Uninstall complete”. DAW projects, OBS scenes and audio device settings
are kept. Setup.app and the downloaded ZIP can then be moved to Trash.

Help: https://yoruhinot.github.io/DawAudioStreamer/en/?lang=en#trouble
Support: https://github.com/yoruhinot/DawAudioStreamer/issues or https://x.com/yoruhinot
Include your macOS, DAW and OBS versions. Check copied details for personal paths before posting.

---

DawAudioStreamer — macOS
=======================
macOS 13以降 · Intel／Apple Silicon（お使いのMacに合うZIPを選んでください）

1. ZIP全体を展開し、DAWとOBSを終了します。
2. DawAudioStreamer Setup.appを開き、［インストール / 更新］を押します。
3. ［インストール完了］を確認してからDAWとOBSを開き直します。
4. DAWのマスターの最後にDAS Sendを1個挿します。
5. OBSのソースへDAS Audio（DAW）を追加します。ソースの設定は不要です。
6. DAWを再生し、OBSのメーターと短い録画で音を確認します。

言語はMacの設定に合わせて自動選択されます。日本語／Englishで変更できます。
更新・削除にも使うのでSetup.appを残しておいてください。Terminal操作は不要です。

macOSにブロックされた場合
-------------------------
一度Setupを開こうとした後、［システム設定］→［プライバシーとセキュリティ］→
［このまま開く］へ進み、［開く］で確認します。信頼できる公式配布物に限る手順です。
マルウェア検出・破損の警告が出た場合は続行せず、サポートへご相談ください。
組織管理のMacでは管理者の許可が必要な場合があります。
https://support.apple.com/ja-jp/102445

完了しなかった場合
------------------
［インストール完了］が表示されるまでは、DAWで探す手順へ進まないでください。
・書き込めない：［Finderで表示］から［情報を見る］→［共有とアクセス権］を確認。
  ご自身のアカウントに書き込み権限がなければ、そのMacの管理者へ相談してください。
  Setupが勝手に権限を変更したり、管理者権限を要求したりすることはありません。
・ファイル不足／検証できない：公式ZIPを再ダウンロードし、全体を展開してください。
・復旧ファイルが残る：削除せず、［詳細をコピー］してサポートへお知らせください。
・後片付けの警告：導入／削除は完了していますが、一時ファイルが残っています。
  再実行する前に、警告の詳細をサポートへお知らせください。

追加される場所（このユーザーのみ）
----------------------------------
~/Library/Audio/Plug-Ins/VST3/DAS Send.vst3
~/Library/Audio/Plug-Ins/Components/DAS Send.component
~/Library/Application Support/obs-studio/plugins/das-obs-source.plugin

DAWに出ない場合は再起動とVST3／AUの再スキャンをお試しください。プラグインがmacOSに
ブロックされた場合は［プライバシーとセキュリティ］と公式ガイドを確認してください。
セキュリティ機能は無効化しないでください。OBSに出ない場合はOBSの再起動と更新を確認。
旧版でOBSに［有効なプロパティがありません］と出ても、設定項目がないだけで正常です。

Discord：macOSでは通常DAS不要です。音声を有効にしてDAWアプリまたは画面を共有し、
視聴する人に音が届くことを確認してください。

削除：DAWとOBSを終了し、Setup.appで［アンインストール］を選びます。
［アンインストール完了］を確認してください。DAWプロジェクト・OBSシーン・音声設定は残ります。
その後、不要なSetup.appとZIPはゴミ箱へ移動できます。

使い方：https://yoruhinot.github.io/DawAudioStreamer/?lang=ja#trouble
報告先：https://github.com/yoruhinot/DawAudioStreamer/issues または https://x.com/yoruhinot
macOS・DAW・OBSのバージョンを添えてください。コピーした詳細の個人名・パスは投稿前に確認してください。
