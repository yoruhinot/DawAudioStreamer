# DawAudioStreamer

[日本語](README.ja.md)

Stream your DAW's audio — ASIO and all — to OBS and Discord.

DawAudioStreamer is a streaming plugin that routes your DAW's master audio to OBS and Discord screen share without touching your audio interface settings. Available as a VST3 for Windows and as an AU/VST3 preview for Intel and Apple Silicon Macs.

> The macOS version is a preview release. Test with a short recording or a private stream before going live.

## Before you start

- **OBS only:** the DawAudioStreamer installer is all you need.
- **Discord on Windows:** VB-CABLE is required. Follow the [Discord setup guide](https://yoruhinot.github.io/DawAudioStreamer/en/?lang=en#discord) if you want screen share audio.

On macOS, the plugin is primarily for OBS. Discord on macOS can use the system's built-in screen share audio, so no virtual audio device is needed.

## Setup

### 1. Install

1. Close OBS, Discord, and your DAW.
2. Download the installer from [Releases](https://github.com/yoruhinot/DawAudioStreamer/releases).
3. Run the Windows installer, or extract the Mac ZIP and open **DawAudioStreamer Setup.app**. Wait for installation to complete.
4. Insert one instance of **DAS Send** at the end of your DAW's master bus.

For Windows Discord support, follow the [VB-CABLE setup guide](https://yoruhinot.github.io/DawAudioStreamer/en/?lang=en#vbcable), then restart Windows. OBS does not require it.

If macOS blocks opening Setup, see the [Mac setup help](https://yoruhinot.github.io/DawAudioStreamer/en/?lang=en#macos-help). Older Mac ZIPs use Install.command.

### 2. OBS

1. In OBS, click **+** under Sources.
2. Add **DAS Audio (DAW)** — no settings needed. “No properties available” in older versions is normal.
3. Play audio and check the OBS audio mixer or a recording.

### 3. Discord

1. Confirm the Discord indicator in DAS Send shows a green `OK`.
2. In Discord, open **Share Screen**.
3. Choose your DAW application to share just the DAW, or choose your full screen to include VST windows too.

Your Discord microphone settings don't need to change.
Ask a viewer to confirm they can hear your audio.

## Plugin status display

| Display | Meaning |
|---|---|
| Green `OK` | OBS is connected / the Discord audio path is ready. Check the recording or viewer's audio too. |
| Gray `WAIT` | Add **DAS Audio (DAW)** as a source in OBS. |
| Yellow `VB-CABLE` | Install VB-CABLE and restart Windows. |
| Yellow `1 ONLY` | Remove extra DAS Send instances — only one allowed. |
| Red `!` | Restart OBS first, then your DAW. |
| Gray, no text | DAS Send is bypassed. |

## Audio doubling

- Insert DAS Send on the master bus once only.
- In OBS, don't capture the same audio through Desktop Audio alongside DAS Audio (DAW).
- In Discord, don't start both an application share and a full-screen share at the same time.

## Audio quality

DAS Send does not alter the audio or volume returned to your DAW. The stream output is converted to stereo 48 kHz, then compressed by OBS or Discord according to their own settings. Keep your DAW master below 0 dBFS to avoid clipping.

## Requirements

- Windows 11 x64
- 64-bit DAW with VST3 support
- OBS Studio x64
- Discord desktop app
- Intel or Apple Silicon Mac running macOS 13 or later (AU/VST3, preview)

Tested with Fender Studio Pro 8, REAPER, Cubase, and Ableton Live 12.

## Uninstall

Close your DAW and OBS, then remove DawAudioStreamer from **Installed apps** in Windows Settings. Your DAW projects, OBS scenes, ASIO settings, and any separately installed virtual audio drivers are not removed.

## Download & contact

Download from [GitHub Releases](https://github.com/yoruhinot/DawAudioStreamer/releases). Windows and macOS may show a security warning on first run.

[Code signing policy](CODE_SIGNING.md)

For bugs or feature requests, reach out on [X (@yoruhinot)](https://x.com/yoruhinot) or open an [Issue](https://github.com/yoruhinot/DawAudioStreamer/issues).

## License

See [LICENSE](LICENSE) for the license and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for third-party attributions.

To build from source, see [build instructions](docs/development.md).
