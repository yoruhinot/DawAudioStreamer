#define MyAppName "DawAudioStreamer"
#ifndef MyAppVersion
#define MyAppVersion "0.4.4"
#endif
#ifndef MyAppFileVersion
#define MyAppFileVersion "0.4.4.0"
#endif
#define MyAppPublisher "yoruhinot"
#define MyAppCopyright "Copyright (c) 2026 yoruhinot"
#define MyAppUrl "https://github.com/yoruhinot/DawAudioStreamer"
#define MyAppSupportUrl "https://github.com/yoruhinot/DawAudioStreamer/issues"
#define MyAppUpdatesUrl "https://github.com/yoruhinot/DawAudioStreamer/releases"
#ifndef BuildRoot
#define BuildRoot "..\build\windows-msvc-release"
#endif
#ifndef SourceArchive
#define SourceArchive "..\build\source\DawAudioStreamer-" + MyAppVersion + "-source.zip"
#endif

[Setup]
AppId={{A2AB3F48-3BA4-46A2-9AE8-E46A6D107BA3}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppUrl}
AppSupportURL={#MyAppSupportUrl}
AppUpdatesURL={#MyAppUpdatesUrl}
AppComments={cm:AppComments}
VersionInfoVersion={#MyAppFileVersion}
VersionInfoProductVersion={#MyAppFileVersion}
VersionInfoDescription=DawAudioStreamer Setup
VersionInfoCompany={#MyAppPublisher}
VersionInfoCopyright={#MyAppCopyright}
DefaultDirName={autopf}\DawAudioStreamer
DefaultGroupName=DawAudioStreamer
DisableProgramGroupPage=yes
OutputDir=..\build\installer
OutputBaseFilename=DawAudioStreamer-Setup-{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName=DawAudioStreamer {#MyAppVersion}
UninstallDisplayIcon={uninstallexe}
CloseApplications=yes
RestartApplications=no
SetupLogging=yes
LanguageDetectionMethod=uilanguage
ShowLanguageDialog=yes
UsePreviousLanguage=yes
LicenseFile=..\LICENSES\AGPL-3.0-only.txt

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[CustomMessages]
english.AppComments=Stream your DAW's ASIO audio to OBS and Discord screen share
english.IconQuickStart=Quick Start
english.IconLicense=License and Source
english.IconUninstall=Uninstall
english.RunQuickStart=Open Quick Start guide

japanese.AppComments=DAWのASIO音声をOBSとDiscordの画面共有へ送ります
japanese.IconQuickStart=クイックスタート
japanese.IconLicense=ライセンスとソース
japanese.IconUninstall=アンインストール
japanese.RunQuickStart=クイックスタートを開く

[Files]
Source: "{#BuildRoot}\plugins\send-vst3\DasSend_artefacts\Release\VST3\DAS Send.vst3\*"; DestDir: "{commoncf64}\VST3\DAS Send.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs restartreplace uninsrestartdelete
Source: "{#BuildRoot}\plugins\obs-source\Release\das-obs-source.dll"; DestDir: "{commonappdata}\obs-studio\plugins\das-obs-source\bin\64bit"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\plugins\obs-source\data\locale\ja-JP.ini"; DestDir: "{commonappdata}\obs-studio\plugins\das-obs-source\data\locale"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\plugins\obs-source\data\locale\en-US.ini"; DestDir: "{commonappdata}\obs-studio\plugins\das-obs-source\data\locale"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\docs\QuickStart.en.txt"; DestDir: "{app}"; DestName: "QuickStart.txt"; Languages: english; Flags: ignoreversion
Source: "..\docs\QuickStart.ja.txt"; DestDir: "{app}"; DestName: "QuickStart.txt"; Languages: japanese; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.ja.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\CODE_SIGNING.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\CODE_SIGNING.ja.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\PRIVACY.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\PRIVACY.ja.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSES\*"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "..\libs\transport\LICENSE"; DestDir: "{app}\licenses"; DestName: "MIT-transport.txt"; Flags: ignoreversion
Source: "..\THIRD_PARTY_NOTICES.md"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "{#BuildRoot}\_deps\juce-src\LICENSE.md"; DestDir: "{app}\licenses"; DestName: "JUCE-LICENSE.md"; Flags: ignoreversion
Source: "{#BuildRoot}\_deps\juce-src\modules\juce_audio_processors\format_types\VST3_SDK\LICENSE.txt"; DestDir: "{app}\licenses"; DestName: "VST3-SDK-LICENSE.txt"; Flags: ignoreversion
Source: "{#BuildRoot}\_deps\juce-src\modules\juce_graphics\fonts\harfbuzz\COPYING"; DestDir: "{app}\licenses"; DestName: "HarfBuzz-COPYING.txt"; Flags: ignoreversion
Source: "{#BuildRoot}\_deps\juce-src\modules\juce_graphics\unicode\sheenbidi\LICENSE"; DestDir: "{app}\licenses"; DestName: "SheenBidi-LICENSE.txt"; Flags: ignoreversion
Source: "{#BuildRoot}\_deps\juce-src\modules\juce_graphics\image_formats\pnglib\LICENSE"; DestDir: "{app}\licenses"; DestName: "libpng-LICENSE.txt"; Flags: ignoreversion
Source: "{#BuildRoot}\_deps\juce-src\modules\juce_graphics\image_formats\jpglib\README"; DestDir: "{app}\licenses"; DestName: "IJG-JPEG-README.txt"; Flags: ignoreversion
Source: "{#BuildRoot}\_deps\juce-src\modules\juce_core\zip\zlib\LICENSE"; DestDir: "{app}\licenses"; DestName: "zlib-LICENSE.txt"; Flags: ignoreversion
Source: "{#BuildRoot}\_deps\obs_headers-src\COPYING"; DestDir: "{app}\licenses"; DestName: "OBS-COPYING.txt"; Flags: ignoreversion
Source: "{#SourceArchive}"; DestDir: "{app}\source"; Flags: ignoreversion

[Icons]
Name: "{group}\{cm:IconQuickStart}"; Filename: "{app}\QuickStart.txt"
Name: "{group}\{cm:IconLicense}"; Filename: "{app}\LICENSE"
Name: "{group}\{cm:IconUninstall}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\QuickStart.txt"; Description: "{cm:RunQuickStart}"; Flags: postinstall shellexec skipifsilent nowait
