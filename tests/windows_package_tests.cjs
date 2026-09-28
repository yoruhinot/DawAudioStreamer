const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const crypto = require('node:crypto');
const { stageWindowsArtifacts } = require('../cmake/StageWindowsArtifacts.cjs');
const read = name => fs.readFileSync(path.join(__dirname, '..', name), 'utf8');
const provenance = { commit: 'a'.repeat(40), event: 'pull_request', ref: 'refs/pull/1/merge',
  runUrl: 'https://github.com/example/project/actions/runs/1', runAttempt: '1', runnerImage: 'test' };

function fixture(t) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'das-windows-package-'));
  t.after(() => fs.rmSync(root, { recursive: true, force: true }));
  for (const dir of ['windows-msvc-release', 'installer', 'source']) {
    fs.mkdirSync(path.join(root, 'build', dir), { recursive: true });
  }
  const cache = path.join(root, 'build/windows-msvc-release/CMakeCache.txt');
  fs.writeFileSync(cache, 'DAS_DISPLAY_VERSION:STRING=0.4.4\r\nDAS_COMPONENT_VERSION:STRING=0.4.4\r\n');
  // Byte fixtures test collection/hashing, not executable or ZIP validity.
  const installer = path.join(root, 'build/installer/DawAudioStreamer-Setup-0.4.4.exe');
  const source = path.join(root, 'build/source/DawAudioStreamer-0.4.4-source.zip');
  fs.writeFileSync(installer, 'installer fixture');
  fs.writeFileSync(source, 'source fixture');
  return { root, cache, installer, source, output: path.join(root, 'build/windows-artifacts') };
}

test('Unsigned package contains matching source, traceable commit and verified hashes', t => {
  const f = fixture(t);
  const info = stageWindowsArtifacts(f.root, provenance);
  assert.equal(info.signing, 'unsigned');
  assert.equal(info.commit, provenance.commit);
  assert.equal(info.event, 'pull_request');
  assert.equal(info.version, '0.4.4');
  assert.equal(info.artifacts.length, 2);
  assert.deepEqual(fs.readdirSync(f.output).sort(),
    ['BUILD-INFO.json', 'DawAudioStreamer-0.4.4-source.zip', 'DawAudioStreamer-Setup-0.4.4.exe', 'SHA256SUMS.txt']);
  assert.deepEqual(JSON.parse(fs.readFileSync(path.join(f.output, 'BUILD-INFO.json'), 'utf8')), info);
  const lines = fs.readFileSync(path.join(f.output, 'SHA256SUMS.txt'), 'utf8').trim().split('\n');
  assert.equal(lines.length, 3);
  for (const line of lines) {
    const [hash, name] = line.split('  ');
    assert.equal(hash, crypto.createHash('sha256').update(fs.readFileSync(path.join(f.output, name))).digest('hex'));
  }
});

test('Missing source fails before creating an artifact directory', t => {
  const f = fixture(t);
  fs.unlinkSync(f.source);
  assert.throws(() => stageWindowsArtifacts(f.root, provenance), /Missing or empty package/);
  assert.equal(fs.existsSync(f.output), false);
});

test('Empty installer fails before staging', t => {
  const f = fixture(t);
  fs.truncateSync(f.installer);
  assert.throws(() => stageWindowsArtifacts(f.root, provenance), /Missing or empty package/);
  assert.equal(fs.existsSync(f.output), false);
});

test('Version mismatch or path-like version cannot select a different file', t => {
  const f = fixture(t);
  for (const value of ['../other', '0.4.3', '0.4.4/other']) {
    fs.writeFileSync(f.cache, 'DAS_DISPLAY_VERSION:STRING=' + value + '\nDAS_COMPONENT_VERSION:STRING=0.4.4\n');
    assert.throws(() => stageWindowsArtifacts(f.root, provenance), /Invalid or inconsistent/);
    assert.equal(fs.existsSync(f.output), false);
  }
});

test('Missing or duplicate CMake metadata fails closed', t => {
  const f = fixture(t);
  for (const value of ['', 'DAS_DISPLAY_VERSION:STRING=0.4.4\nDAS_DISPLAY_VERSION:STRING=0.4.4\n']) {
    fs.writeFileSync(f.cache, value);
    assert.throws(() => stageWindowsArtifacts(f.root, provenance), /Missing or ambiguous/);
  }
});

test('Stale artifact directory is never overwritten', t => {
  const f = fixture(t);
  stageWindowsArtifacts(f.root, provenance);
  const original = fs.readFileSync(path.join(f.output, 'BUILD-INFO.json'));
  assert.throws(() => stageWindowsArtifacts(f.root, { ...provenance, commit: 'b'.repeat(40) }), /already exists/);
  assert.deepEqual(fs.readFileSync(path.join(f.output, 'BUILD-INFO.json')), original);
});

test('An untraceable build is not collected', t => {
  const f = fixture(t);
  assert.throws(() => stageWindowsArtifacts(f.root, { ...provenance, commit: 'unknown' }), /full Git commit/);
});

test('CMake supplies actual build paths and versions to Inno Setup', () => {
  const cmake = read('CMakeLists.txt');
  const iss = read('installer/DawAudioStreamer.iss');
  for (const key of ['BuildRoot', 'SourceArchive', 'MyAppVersion', 'MyAppFileVersion']) {
    assert.ok(iss.includes('#ifndef ' + key));
    assert.ok(cmake.includes('/D' + key + '='));
  }
  assert.ok(cmake.includes('/DBuildRoot=$' + '{CMAKE_BINARY_DIR}'));
  assert.ok(cmake.includes('/DSourceArchive=$' + '{DAS_SOURCE_ARCHIVE}'));
  const version = cmake.match(/project\(DawAudioStreamer VERSION ([\d.]+)/)[1];
  assert.ok(iss.includes('#define MyAppVersion "' + version + '"'));
  assert.ok(iss.includes('#define MyAppFileVersion "' + version + '.0"'));
  const source = read('cmake/CreateSourcePackage.cmake');
  assert.ok(source.includes('foreach(directory .github '));
  for (const file of ['CODE_SIGNING.md', 'CODE_SIGNING.ja.md']) {
    assert.ok(source.includes(file));
    assert.ok(iss.includes(file));
  }
});

test('CI builds and tests before packaging, with no signing or release credentials', () => {
  const ci = read('.github/workflows/windows.yml');
  assert.match(ci, /runs-on: windows-2025/);
  assert.match(ci, /contents: read/);
  assert.match(ci, /persist-credentials: false/);
  assert.match(ci, /cmake --preset windows-msvc-release/);
  assert.match(ci, /if-no-files-found: error/);
  assert.match(ci, /name: DawAudioStreamer-Windows-unsigned-/);
  assert.doesNotMatch(ci, /pull_request_target|secrets\.|contents: write|gh release|submit-signing-request/);
  const order = ['ctest --preset', '--target das_installer', 'node cmake/StageWindowsArtifacts.cjs', 'uses: actions/upload-artifact@'];
  for (let i = 1; i < order.length; i++) assert.ok(ci.indexOf(order[i]) > ci.indexOf(order[i - 1]));
  for (const action of ci.matchAll(/uses: ([^\s]+)/g)) assert.match(action[1], /@[a-f0-9]{40}$/);
});

test('Both languages link a truthful policy without changing the download warnings', () => {
  for (const [file, policy] of [['site/index.html', 'CODE_SIGNING.ja.md'], ['site/en/index.html', 'CODE_SIGNING.md']]) {
    const html = read(file);
    const footer = html.split('<footer>')[1].split('</footer>')[0];
    assert.ok(footer.includes('/blob/main/' + policy));
    assert.ok(footer.includes('Code signing policy'));
    assert.match(html, /警告|security warning/);
  }
  assert.match(read('CODE_SIGNING.md'), /currently unsigned/);
  assert.match(read('CODE_SIGNING.md'), /preparing a new application/);
  assert.match(read('CODE_SIGNING.ja.md'), /未署名/);
  assert.match(read('CODE_SIGNING.ja.md'), /再申請を準備/);
  for (const file of ['CODE_SIGNING.md', 'CODE_SIGNING.ja.md']) {
    assert.match(read(file), /yoruhinot/);
    assert.doesNotMatch(read(file), /[\w.+-]+@[\w.-]+\.[A-Za-z]{2,}/);
    assert.doesNotMatch(read(file), /Free code signing provided by/);
  }
  assert.match(read('PRIVACY.md'), /Windows and macOS/);
  assert.match(read('PRIVACY.md'), /browser/);
  assert.match(read('PRIVACY.ja.md'), /Windows・macOS/);
  assert.match(read('PRIVACY.ja.md'), /ブラウザー/);
});
