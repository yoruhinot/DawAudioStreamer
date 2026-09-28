// Collect an unsigned CI build; this script never signs or publishes a release.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { execFileSync } = require('node:child_process');

function cacheValue(cache, key) {
  const matches = [...cache.matchAll(new RegExp('^' + key + ':[^=]+=(.+)$', 'gm'))];
  if (matches.length !== 1) throw new Error('Missing or ambiguous CMake value: ' + key);
  return matches[0][1].trim();
}

function stageWindowsArtifacts(root, provenance) {
  const cache = fs.readFileSync(path.join(root, 'build/windows-msvc-release/CMakeCache.txt'), 'utf8');
  const version = cacheValue(cache, 'DAS_DISPLAY_VERSION');
  const componentVersion = cacheValue(cache, 'DAS_COMPONENT_VERSION');
  if (!/^\d+\.\d+\.\d+(?:-[A-Za-z0-9.-]+)?$/.test(version) ||
      !/^\d+\.\d+\.\d+$/.test(componentVersion) ||
      version.split('-')[0] !== componentVersion) {
    throw new Error('Invalid or inconsistent package versions');
  }
  if (!/^[a-f0-9]{40}$/.test(provenance.commit)) throw new Error('A full Git commit SHA is required');
  const output = path.join(root, 'build/windows-artifacts');
  if (fs.existsSync(output)) throw new Error('Artifact directory already exists; use a clean build directory');
  const files = [
    path.join(root, 'build/installer', 'DawAudioStreamer-Setup-' + version + '.exe'),
    path.join(root, 'build/source', 'DawAudioStreamer-' + version + '-source.zip')
  ];
  // Check everything before writing: a missing source ZIP must fail the build.
  for (const file of files) {
    if (!fs.existsSync(file) || !fs.statSync(file).isFile() || fs.statSync(file).size === 0) {
      throw new Error('Missing or empty package file: ' + file);
    }
  }
  fs.mkdirSync(output);
  const sha256 = file => crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
  const artifacts = files.map(file => {
    const name = path.basename(file);
    const destination = path.join(output, name);
    fs.copyFileSync(file, destination, fs.constants.COPYFILE_EXCL);
    return { name, sha256: sha256(destination), bytes: fs.statSync(destination).size };
  });
  const info = {
    product: 'DawAudioStreamer', version, componentVersion,
    signing: 'unsigned', ...provenance, artifacts
  };
  const infoPath = path.join(output, 'BUILD-INFO.json');
  fs.writeFileSync(infoPath, JSON.stringify(info, null, 2) + '\n', { flag: 'wx' });
  const hashes = [...artifacts, { name: 'BUILD-INFO.json', sha256: sha256(infoPath) }];
  fs.writeFileSync(path.join(output, 'SHA256SUMS.txt'),
    hashes.map(file => file.sha256 + '  ' + file.name).join('\n') + '\n', { flag: 'wx' });
  return info;
}

if (require.main === module) {
  const root = path.resolve(__dirname, '..');
  const commit = execFileSync('git', ['rev-parse', 'HEAD'], { cwd: root, encoding: 'utf8' }).trim();
  if (process.env.GITHUB_SHA && process.env.GITHUB_SHA !== commit) {
    throw new Error('The checkout does not match the workflow commit');
  }
  const info = stageWindowsArtifacts(root, {
    commit,
    ref: process.env.GITHUB_REF || null,
    event: process.env.GITHUB_EVENT_NAME || 'local',
    runUrl: process.env.GITHUB_RUN_ID
      ? process.env.GITHUB_SERVER_URL + '/' + process.env.GITHUB_REPOSITORY + '/actions/runs/' + process.env.GITHUB_RUN_ID
      : null,
    runAttempt: process.env.GITHUB_RUN_ATTEMPT || null,
    runnerImage: process.env.ImageVersion || null
  });
  console.log('Prepared unsigned Windows package ' + info.version + ' from ' + info.commit);
}
module.exports = { stageWindowsArtifacts };
