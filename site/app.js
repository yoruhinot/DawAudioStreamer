const downloadButton = document.querySelector("#windows-download");
const downloadTitle = document.querySelector("#download-title");
const versionLabel = document.querySelector("#download-version");
const releaseNotice = document.querySelector("#release-notice");

fetch("https://api.github.com/repos/yoruhinot/DawAudioStreamer/releases?per_page=10", {
  headers: { Accept: "application/vnd.github+json" }
})
  .then((response) => {
    if (!response.ok) throw new Error("Release information is unavailable");
    return response.json();
  })
  .then((releases) => {
    const release = releases.find((item) =>
      !item.draft && item.assets.some((asset) => asset.name.toLowerCase().endsWith(".exe"))
    );
    const installer = release?.assets.find((asset) => asset.name.toLowerCase().endsWith(".exe"));
    if (!release || !installer) return;
    downloadButton.href = installer.browser_download_url;
    downloadTitle.textContent = release.prerelease
      ? "Windowsベータ版をダウンロード"
      : "Windows版をダウンロード";
    versionLabel.textContent = `${release.tag_name}・Windows 11・64-bit`;
    if (!release.prerelease) releaseNotice.hidden = true;
  })
  .catch(() => {
    // The button already falls back to the latest GitHub release page.
  });
