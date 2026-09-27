// Packaging/UI contracts runnable on Windows too. Native behavior is tested in macos_setup_tests.swift.
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const root = path.join(__dirname, '..');
const read = name => fs.readFileSync(path.join(root, name), 'utf8');

test('Windows completion opens one optional guide, not a driver page', () => {
  const iss = read('installer/DawAudioStreamer.iss');
  const run = iss.split('[Run]')[1];
  assert.equal((run.match(/^Filename:/gm) || []).length, 1);
  assert.match(run, /QuickStart\.txt/);
  assert.doesNotMatch(iss, /CreateOutputMsgPage|InfoAfterFile|RunVbCable/);
  for (const lang of ['ja-JP', 'en-US']) assert.ok(iss.includes(`locale\\${lang}.ini`));
});

test('Both OBS builds skip the empty properties dialog and have localized help', () => {
  const source = read('plugins/obs-source/Source/obs-plugin.cpp');
  assert.match(source, /OBS_SOURCE_CAP_DONT_SHOW_PROPERTIES/);
  assert.match(source, /\.get_properties = sourceProperties/);
  assert.match(source, /obs_module_text\("SetupHelp"\), OBS_TEXT_INFO/);
  for (const lang of ['ja-JP', 'en-US']) {
    assert.match(read(`plugins/obs-source/data/locale/${lang}.ini`), /^SetupHelp=".+"$/m);
    assert.ok(read('plugins/obs-source/CMakeLists.txt').includes(`data/locale/${lang}.ini`));
  }
  const imports = read('plugins/obs-source/obs-import.def');
  assert.match(imports, /obs_properties_create/);
  assert.match(imports, /obs_properties_add_text/);
});

test('Mac package contains a native bilingual app and no terminal entry points', () => {
  const pack = read('cmake/CreateMacPreviewPackage.cmake');
  assert.match(pack, /DawAudioStreamer Setup\.app/);
  assert.match(pack, /Contents\/Resources\/payload/);
  assert.match(pack, /xcrun swiftc/);
  assert.doesNotMatch(pack, /Install\.command|Uninstall\.command|Language\.zsh/);
  const app = read('installer/macos/SetupApp.swift');
  assert.match(app, /Locale\.preferredLanguages/);
  assert.match(app, /日本語.*English/);
  assert.match(app, /インストール完了/);
  assert.match(app, /Setup did not complete/);
  for (const name of ['SetupApp.swift', 'SetupCore.swift']) {
    assert.doesNotMatch(read(`installer/macos/${name}`), /\b(?:sudo|chmod|chown|xattr|spctl)\b/);
  }
});

test('Mac CI exercises isolated tests and the extracted release payload on both architectures', () => {
  const ci = read('.github/workflows/macos-preview.yml');
  assert.match(ci, /label: AppleSilicon/);
  assert.match(ci, /label: Intel/);
  assert.match(ci, /tests\/macos_setup_tests.swift/);
  assert.match(ci, /test-setup "build\/setup-archive-check/);
  assert.doesNotMatch(ci, /macos_language_tests/);
});

test('Mac guide has a bilingual filename in the package and extracted ZIP checks', () => {
  const guide = 'README_はじめにお読みください.txt';
  const pack = read('cmake/CreateMacPreviewPackage.cmake');
  assert.ok(pack.includes(`"\${package_root}/${guide}" COPYONLY`));
  assert.ok(read('.github/workflows/macos-preview.yml').includes(`test -f "$package/${guide}"`));
  assert.ok(read('.github/workflows/macos-preview.yml').includes(`cmp installer/macos/README-macOS.txt "build/setup-archive-check/$(basename "$package")/${guide}"`));
  const text = read('installer/macos/README-macOS.txt');
  assert.match(text, /日本語の手順は、このファイルの後半/);
  assert.match(text, /Installation complete/);
  assert.match(text, /インストール完了/);
});

test('Setup uses the product name; OBS confirmation methods are alternatives', () => {
  const app = read('installer/macos/SetupApp.swift');
  assert.match(app, /DawAudioStreamerをこのMacに追加/);
  assert.match(app, /Add DawAudioStreamer to this Mac/);
  assert.doesNotMatch(app, /DASを|Add DAS to this Mac|Uninstall DAS\?/);
  assert.match(read('plugins/obs-source/data/locale/ja-JP.ini'), /音声ミキサーや録画などで音を確認できます/);
  assert.match(read('plugins/obs-source/data/locale/en-US.ini'), /audio mixer or a recording/);
});
