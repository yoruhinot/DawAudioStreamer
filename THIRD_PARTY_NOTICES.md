# 第三者ソフトウェアと権利表示

配布ソースアーカイブには、バイナリのビルドに使用した次の固定revisionを収録します。

## JUCE

- 上流：<https://github.com/juce-framework/JUCE>
- revision：`3af3ce009f6a02f6fa651008fffb5b41743a9fab`（JUCE 8.0.10）
- ライセンス：JUCE Framework modulesはAGPLv3とJUCE商用ライセンスのデュアルライセンス

この無料配布版はJUCE商用ライセンスを前提とせず、JUCEをリンクするDAS Sendを
AGPL-3.0-only条件で提供します。対応ソースには開発者向け確認用のDAS Engineも含み、同じ条件で
提供します。JUCE自身の`LICENSE.md`と、使用するJUCE依存物の
ライセンスをインストーラーと対応ソースへ収録します。配布バイナリに組み込まれるHarfBuzz、
SheenBidi、libpng、Independent JPEG Group JPEG、zlibの表示も`licenses`へ収録します。

## Steinberg VST3 SDK

JUCE 8.0.10に同梱されたVST3 SDKヘッダーを、VST3プラグインのビルドに使用します。対応ソースと
`VST3-SDK-LICENSE.txt`を配布物へ収録します。

VST is a registered trademark of Steinberg Media Technologies GmbH.

## r8brain-free-src

- 上流：<https://github.com/avaneev/r8brain-free-src>
- revision：`8fff6f3db26f14a8f5e8fb871000613673db5753`
- ライセンス：MIT License
- Copyright (c) 2013-2026 Aleksey Vaneev

Sample rate converter designed by Aleksey Vaneev of Voxengo.

ライセンス全文は`LICENSES/MIT-r8brain.txt`に収録します。

## SIMDe

- 上流：<https://github.com/simd-everywhere/simde>
- revision：`71fd833d9666141edcd1d3c109a80e228303d8d7`（0.8.2）
- ライセンス：MIT License

macOS版OBSプラグインのApple Silicon対応にSIMDeヘッダーを使用します。
ライセンス全文はmacOS配布物の`licenses/SIMDe`に収録します。

## OBS Studio

- 上流：<https://github.com/obsproject/obs-studio>
- revision：`c17423ce05899ecb93f678601b3feaa8a469b180`（OBS Studio 32.1.2）
- ライセンス：GPL-2.0-or-later

OBS音声ソースはこのrevisionの公開APIヘッダーでビルドし、実行時に利用者がインストールした
OBS Studioへ動的リンクします。OBS Studio本体は同梱しません。参照revisionの`COPYING`と
対応ソースを配布物へ収録します。

## Inno Setup

- 上流：<https://jrsoftware.org/isinfo.php>
- ライセンス：Inno Setup License

Windowsインストーラーの生成にInno Setupを使用します。ライセンス全文は
`LICENSES/Inno-Setup.txt`に収録します。

## 製品名・商標

DawAudioStreamerは、OBS Project、Discord Inc.、Steinberg Media Technologies GmbH、
Elgato、VB-Audioの公式製品ではなく、各社からの承認、提携、保証を意味しません。
OBS、Discord、JUCE、VST、ASIO、Elgato、VB-Audioなどの名称および商標は各権利者に帰属します。
本ソフトウェアはDiscordの非公開API、非公開イベント、DLL注入を使用しません。
