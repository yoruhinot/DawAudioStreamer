# Code signing policy

[日本語](CODE_SIGNING.ja.md)

## Current status

Windows releases are currently unsigned. We are preparing a new application to [SignPath Foundation](https://signpath.org/); approval and signing are not yet in place.

macOS releases use ad-hoc signatures, not Apple Developer ID signatures or notarization. This is unchanged.

## Windows signing plan

- Scope: DawAudioStreamer's Windows installer and its own bundled plug-in binaries. Separately distributed software such as VB-CABLE is not included.
- Builds: only artifacts built from this public repository on GitHub-hosted runners will be submitted for signing. Pull-request builds are for verification, not release signing.
- Author, reviewer and signing approver: [夜人 / yoruhinot](https://github.com/yoruhinot). External pull requests must be reviewed before merging. Each production signing request will require the maintainer's explicit approval.
- Before signing is enabled, the signing account and repository access must use multi-factor authentication, and the permitted branch, product metadata and artifact contents must be configured and verified in SignPath.
- If accepted, signatures will identify **SignPath Foundation**, not the maintainer's activity name. The official attribution will be added when the service is available; no current sponsorship is claimed.

Official downloads: [GitHub Releases](https://github.com/yoruhinot/DawAudioStreamer/releases). CI artifacts are unsigned test builds and are not published releases.

[Privacy policy](PRIVACY.md)
