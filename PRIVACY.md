# Privacy

[日本語](PRIVACY.ja.md)

DawAudioStreamer has no telemetry, usage analytics, crash reporting, auto-update, ads, or account features. Its audio components do not send data over the network.

On Windows and macOS, DAW audio is passed locally to the OBS source through shared memory for the same user. Windows also supports a local audio path for Discord screen sharing. DawAudioStreamer does not record audio files or read screen images; OBS and Discord handle recording, screen capture and streaming.

Opening a help link in Setup launches your browser. Visiting the website or downloading files connects to the respective hosting services. The website requests release information from GitHub and stores your chosen language locally in your browser. This is separate from the plug-ins' local audio processing.

Information sent through those services is subject to their policies: [GitHub](https://docs.github.com/en/site-policy/privacy-policies/github-general-privacy-statement), [OBS](https://obsproject.com/privacy-policy), [Discord](https://discord.com/privacy), and your chosen streaming provider. DawAudioStreamer does not modify Discord's microphone settings, your DAW's audio-interface configuration, or Windows default audio devices.
