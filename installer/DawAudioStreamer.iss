#define MyAppName "DawAudioStreamer"
#define MyAppVersion "0.4.0-beta.3"
#define MyAppFileVersion "0.4.0.0"
#define MyAppPublisher "yoruhinot"
#define MyAppCopyright "Copyright (c) 2026 yoruhinot"
#define MyAppUrl "https://github.com/yoruhinot/DawAudioStreamer"
#define MyAppSupportUrl "https://github.com/yoruhinot/DawAudioStreamer/issues"
#define MyAppUpdatesUrl "https://github.com/yoruhinot/DawAudioStreamer/releases"
#define BuildRoot "..\build\windows-msvc-release"
#define SourceArchive "..\build\source\DawAudioStreamer-0.4.0-beta.3-source.zip"
#define VbCableUrl "https://yoruhinot.github.io/DawAudioStreamer/#vbcable"

[Setup]
AppId={{A2AB3F48-3BA4-46A2-9AE8-E46A6D107BA3}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppUrl}
AppSupportURL={#MyAppSupportUrl}
AppUpdatesURL={#MyAppUpdatesUrl}
AppComments=DAWのASIO音声をOBSとDiscordの画面共有へ送ります
VersionInfoVersion={#MyAppFileVersion}
VersionInfoProductVersion={#MyAppFileVersion}
VersionInfoDescription=DawAudioStreamer セットアップ
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
LicenseFile=..\LICENSES\AGPL-3.0-only.txt
InfoAfterFile=..\docs\クイックスタート.txt

[Languages]
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Files]
Source: "{#BuildRoot}\plugins\send-vst3\Release\das-virtual-audio-check.exe"; DestDir: "{tmp}"; Flags: dontcopy
Source: "{#BuildRoot}\plugins\send-vst3\DasSend_artefacts\Release\VST3\DAS Send.vst3\*"; DestDir: "{commoncf64}\VST3\DAS Send.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs restartreplace uninsrestartdelete
Source: "{#BuildRoot}\plugins\obs-source\Release\das-obs-source.dll"; DestDir: "{commonappdata}\obs-studio\plugins\das-obs-source\bin\64bit"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\plugins\obs-source\data\locale\ja-JP.ini"; DestDir: "{commonappdata}\obs-studio\plugins\das-obs-source\data\locale"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\docs\クイックスタート.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\PRIVACY.md"; DestDir: "{app}"; Flags: ignoreversion
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
Name: "{group}\クイックスタート"; Filename: "{app}\クイックスタート.txt"
Name: "{group}\ライセンスとソース"; Filename: "{app}\LICENSE"
Name: "{group}\アンインストール"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\クイックスタート.txt"; Description: "クイックスタートを開く"; Flags: postinstall shellexec skipifsilent nowait
Filename: "{#VbCableUrl}"; Description: "Discord用のVB-CABLE導入手順を開く"; Flags: postinstall shellexec skipifsilent; Check: ShouldOfferVbCable

[Code]
var
  VirtualAudioAvailable: Boolean;
  VirtualAudioCheckFailed: Boolean;
  VirtualAudioPage: TOutputMsgWizardPage;

function ShouldOfferVbCable: Boolean;
begin
  Result := not VirtualAudioAvailable;
end;

procedure InitializeWizard;
var
  CheckResult: Integer;
  PageDescription: String;
  PageMessage: String;
begin
  VirtualAudioAvailable := False;
  VirtualAudioCheckFailed := False;
  try
    ExtractTemporaryFile('das-virtual-audio-check.exe');
    if Exec(ExpandConstant('{tmp}\das-virtual-audio-check.exe'), '', '', SW_HIDE,
            ewWaitUntilTerminated, CheckResult) then
    begin
      VirtualAudioAvailable := CheckResult = 0;
      VirtualAudioCheckFailed := CheckResult = 2;
    end
    else
      VirtualAudioCheckFailed := True;
  except
    VirtualAudioCheckFailed := True;
  end;

  if VirtualAudioAvailable then
  begin
    PageDescription := '対応する無音仮想出力が見つかりました';
    PageMessage :=
      'OBSとDiscordの両方を使用できます。' + #13#10 + #13#10 +
      'DawAudioStreamerは既定の音声デバイス、ASIO設定、Discordのマイク設定を変更しません。';
  end
  else if VirtualAudioCheckFailed then
  begin
    PageDescription := 'Discordを使う場合はVB-CABLEを追加してください';
    PageMessage :=
      'インストールは続行できます。OBSはそのまま使用できます。' + #13#10 + #13#10 +
      'Discordで音声を共有するにはVB-CABLEが必要です。' + #13#10 +
      '完了を押すと詳しい導入手順を開きます。不要な場合は完了画面でチェックを外せます。';
  end
  else
  begin
    PageDescription := 'Discordを使う場合はVB-CABLEを追加してください';
    PageMessage :=
      'このPCでは対応する無音仮想出力が見つかりませんでした。' + #13#10 + #13#10 +
      '・OBSはこのまま使用できます。' + #13#10 +
      '・Discordの画面共有音声にはVB-CABLEが必要です。' + #13#10 +
      '・本セットアップは第三者ドライバーや既定の音声設定を変更しません。' + #13#10 + #13#10 +
      '完了を押すと詳しい導入手順を開きます。不要な場合は完了画面でチェックを外せます。';
  end;

  VirtualAudioPage := CreateOutputMsgPage(wpLicense, 'Discord用音声の確認',
                                           PageDescription, PageMessage);
end;
