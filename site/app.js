const downloadButton = document.querySelector("#windows-download");
const downloadTitle = document.querySelector("#download-title");
const versionLabel = document.querySelector("#download-version");
const macPicker = document.querySelector("#mac-download");
const macDownloadArm = document.querySelector("#mac-download-arm");
const macDownloadIntel = document.querySelector("#mac-download-intel");

// A troubleshooting link should reveal the answer, not just a collapsed heading.
const macHelp = document.querySelector("#macos-help");
if (macHelp) {
  const revealMacHelp = () => {
    if (window.location.hash === "#macos-help") macHelp.open = true;
  };
  window.addEventListener("hashchange", revealMacHelp);
  document.querySelectorAll('a[href="#macos-help"]').forEach(link => {
    link.addEventListener("click", () => { macHelp.open = true; });
  });
  revealMacHelp();
}

if (macPicker) {
  document.addEventListener("click", (event) => {
    if (macPicker.open && !macPicker.contains(event.target)) macPicker.open = false;
  });
  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && macPicker.open) macPicker.open = false;
  });
}
const demoVideo = document.querySelector("#demo-video");
const demoPlay = document.querySelector("#demo-play");

if (demoVideo && demoPlay) {
  demoPlay.hidden = false;
  demoPlay.addEventListener("click", () => {
    demoPlay.hidden = true;
    demoVideo.focus({ preventScroll: true });
    demoVideo.play().catch(() => {
      demoPlay.hidden = false;
    });
  });
  demoVideo.addEventListener("play", () => { demoPlay.hidden = true; });
  demoVideo.addEventListener("ended", () => { demoPlay.hidden = false; });
}

const isEn = document.documentElement.lang === "en";

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
    if (release && installer) {
      downloadButton.href = installer.browser_download_url;
      downloadTitle.textContent = isEn
        ? (release.prerelease ? "Download Windows Beta" : "Download for Windows")
        : (release.prerelease ? "Windowsベータ版をダウンロード" : "Windows版をダウンロード");
      versionLabel.textContent = isEn
        ? `${release.tag_name} · Windows 11 · x64`
        : `${release.tag_name}・Windows 11・x64`;
    }

    const findMacAsset = (item, arch) => item.assets.find((asset) => {
      const name = asset.name.toLowerCase();
      return name.endsWith(".zip") && name.includes(arch);
    });
    // Architectures may be published separately; resolve each independently.
    for (const [arch, button] of [["macos-applesilicon", macDownloadArm], ["macos-intel", macDownloadIntel]]) {
      const macRelease = releases.find((item) => !item.draft && findMacAsset(item, arch));
      const asset = macRelease && findMacAsset(macRelease, arch);
      if (!asset || !button) continue;
      button.href = asset.browser_download_url;
      button.removeAttribute("aria-disabled");
      button.removeAttribute("tabindex");
      button.querySelector("small").textContent = isEn
        ? `${macRelease.tag_name} · macOS 13+`
        : `${macRelease.tag_name}・macOS 13以降`;
    }
  })
  .catch(() => {
    // Keep the published download links when release information is unavailable.
  });
