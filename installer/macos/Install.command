#!/bin/zsh
set -euo pipefail

SCRIPT_DIR="${0:A:h}"
PAYLOAD_DIR="${SCRIPT_DIR}/payload"
VST3_TARGET="${HOME}/Library/Audio/Plug-Ins/VST3/DAS Send.vst3"
AU_TARGET="${HOME}/Library/Audio/Plug-Ins/Components/DAS Send.component"
OBS_TARGET="${HOME}/Library/Application Support/obs-studio/plugins/das-obs-source.plugin"

print "DawAudioStreamer macOS検証版をインストールします。"
print "DAWとOBSを終了してから続けてください。"
print

if [[ ! -d "${PAYLOAD_DIR}/DAS Send.vst3" ||
      ! -d "${PAYLOAD_DIR}/DAS Send.component" ||
      ! -d "${PAYLOAD_DIR}/das-obs-source.plugin" ]]; then
  print -u2 "必要なファイルが見つかりません。ZIPを展開してから実行してください。"
  read -k 1 "?何かキーを押すと閉じます。"
  exit 1
fi

mkdir -p "${VST3_TARGET:h}" "${AU_TARGET:h}" "${OBS_TARGET:h}"
rm -rf "${VST3_TARGET}" "${AU_TARGET}" "${OBS_TARGET}"
/usr/bin/ditto "${PAYLOAD_DIR}/DAS Send.vst3" "${VST3_TARGET}"
/usr/bin/ditto "${PAYLOAD_DIR}/DAS Send.component" "${AU_TARGET}"
/usr/bin/ditto "${PAYLOAD_DIR}/das-obs-source.plugin" "${OBS_TARGET}"

/usr/bin/xattr -dr com.apple.quarantine "${VST3_TARGET}" "${AU_TARGET}" "${OBS_TARGET}" 2>/dev/null || true
/usr/bin/killall -u "${USER}" AudioComponentRegistrar 2>/dev/null || true

print
print "インストールが完了しました。"
print "1. DAWのマスターへ「DAS Send」を1個挿します。"
print "2. OBSのソースへ「DAS Audio（DAW）」を追加します。"
print "3. DiscordではDAWアプリまたは画面全体を共有します。"
print
read -k 1 "?何かキーを押すと閉じます。"
print
