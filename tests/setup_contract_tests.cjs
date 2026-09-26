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
