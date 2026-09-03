#!/bin/zsh
set -euo pipefail

VST3_TARGET="${HOME}/Library/Audio/Plug-Ins/VST3/DAS Send.vst3"
AU_TARGET="${HOME}/Library/Audio/Plug-Ins/Components/DAS Send.component"
OBS_TARGET="${HOME}/Library/Application Support/obs-studio/plugins/das-obs-source.plugin"

print "DawAudioStreamer macOS検証版をアンインストールします。"
print "DAWとOBSを終了してから続けてください。"
print

rm -rf "${VST3_TARGET}" "${AU_TARGET}" "${OBS_TARGET}"
/usr/bin/killall -u "${USER}" AudioComponentRegistrar 2>/dev/null || true

print "アンインストールが完了しました。DAWプロジェクトとOBSシーンは削除していません。"
read -k 1 "?何かキーを押すと閉じます。"
print
