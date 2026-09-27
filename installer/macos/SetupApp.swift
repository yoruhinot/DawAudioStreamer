// SPDX-License-Identifier: AGPL-3.0-only
import AppKit
import Darwin

@main
final class SetupApp: NSObject, NSApplicationDelegate {
    private var window: NSWindow!
    private var language: NSSegmentedControl!
    private var titleLabel: NSTextField!
    private var body: NSTextField!
    private var installButton: NSButton!
    private var removeButton: NSButton!
    private var guideButton: NSButton!
    private var progress: NSProgressIndicator!
    private var busy = false
    private var japanese = Locale.preferredLanguages.first?.hasPrefix("ja") ?? false
    private let fm = FileManager.default
    private func t(_ ja: String, _ en: String) -> String { japanese ? ja : en }

    static func main() {
        let app = NSApplication.shared
        let delegate = SetupApp()
        app.delegate = delegate
        app.setActivationPolicy(.regular)
        withExtendedLifetime(delegate) { app.run() }
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        let menu = NSMenu()
        let appMenu = NSMenu()
        appMenu.addItem(withTitle: "Quit Setup", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
        let menuItem = NSMenuItem()
        menuItem.submenu = appMenu
        menu.addItem(menuItem)
        NSApp.mainMenu = menu
        window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 570, height: 460),
                          styleMask: [.titled, .closable, .miniaturizable], backing: .buffered, defer: false)
        window.title = "DawAudioStreamer Setup"
        window.isReleasedWhenClosed = false
        let stack = NSStackView()
        stack.orientation = .vertical
        stack.alignment = .leading
        stack.spacing = 20
        stack.translatesAutoresizingMaskIntoConstraints = false
        window.contentView!.addSubview(stack)
        NSLayoutConstraint.activate([
            stack.leadingAnchor.constraint(equalTo: window.contentView!.leadingAnchor, constant: 30),
            stack.trailingAnchor.constraint(equalTo: window.contentView!.trailingAnchor, constant: -30),
            stack.topAnchor.constraint(equalTo: window.contentView!.topAnchor, constant: 26)
        ])
        language = NSSegmentedControl(labels: ["日本語", "English"], trackingMode: .selectOne, target: self, action: #selector(changeLanguage))
        language.selectedSegment = japanese ? 0 : 1
        stack.addArrangedSubview(language)
        titleLabel = NSTextField(wrappingLabelWithString: "")
        titleLabel.font = .boldSystemFont(ofSize: 23)
        titleLabel.setContentCompressionResistancePriority(.required, for: .vertical)
        stack.addArrangedSubview(titleLabel)
        titleLabel.widthAnchor.constraint(equalTo: stack.widthAnchor).isActive = true
        body = NSTextField(wrappingLabelWithString: "")
        body.font = .systemFont(ofSize: 14)
        body.setContentCompressionResistancePriority(.required, for: .vertical)
        stack.addArrangedSubview(body)
        body.widthAnchor.constraint(equalTo: stack.widthAnchor).isActive = true
        progress = NSProgressIndicator()
        progress.style = .bar
        progress.isIndeterminate = true
        progress.isHidden = true
        stack.addArrangedSubview(progress)
        progress.widthAnchor.constraint(equalTo: stack.widthAnchor).isActive = true
        installButton = NSButton(title: "", target: self, action: #selector(install))
        installButton.keyEquivalent = "\r"
        removeButton = NSButton(title: "", target: self, action: #selector(uninstall))
        let buttons = NSStackView(views: [installButton, removeButton])
        buttons.spacing = 12
        stack.addArrangedSubview(buttons)
        guideButton = NSButton(title: "", target: self, action: #selector(guide))
        stack.addArrangedSubview(guideButton)
        refresh()
        window.center()
        window.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)
    }

    @objc private func changeLanguage() { japanese = language.selectedSegment == 0; refresh() }
    private func refresh() {
        titleLabel.stringValue = t("DawAudioStreamerをこのMacに追加", "Add DawAudioStreamer to this Mac")
        body.stringValue = t("DAWとOBSを終了してから進めてください。\n\nこのユーザーにVST3・AU・OBSプラグインをインストールします。音声デバイスやDAWの設定は変更しません。",
                             "Close your DAW and OBS before continuing.\n\nInstall VST3, AU and OBS plugins for this user. Audio devices and DAW settings are not changed.")
        installButton.title = t("インストール / 更新", "Install / Update")
        removeButton.title = t("アンインストール…", "Uninstall…")
        guideButton.title = t("使い方・困ったとき", "Setup guide & help")
    }
    @objc private func guide() {
        NSWorkspace.shared.open(URL(string: "https://yoruhinot.github.io/DawAudioStreamer/" + (japanese ? "?lang=ja" : "en/?lang=en") + "#macos")!)
    }
    @objc private func install() { run(install: true) }
    @objc private func uninstall() {
        let alert = NSAlert()
        alert.messageText = t("DawAudioStreamerをアンインストールしますか？", "Uninstall DawAudioStreamer?")
        alert.informativeText = t("このユーザーのDAS Send（VST3・AU）とDAS Audio（OBS）だけを削除します。DAWプロジェクト・OBSシーンは残ります。DAWとOBSを終了してください。",
                                 "Remove only this user's DAS Send (VST3 / AU) and DAS Audio (OBS). DAW projects and OBS scenes are kept. Close your DAW and OBS first.")
        alert.addButton(withTitle: t("キャンセル", "Cancel"))
        alert.addButton(withTitle: t("アンインストール", "Uninstall"))
        if alert.runModal() == .alertSecondButtonReturn { run(install: false) }
    }

    private func run(install: Bool) {
        guard !busy else { return }
        guard geteuid() != 0 else {
            showFailure(SetupIssue(kind: "root", path: fm.homeDirectoryForCurrentUser, detail: "Run Setup from Finder as your normal user, not root."))
            return
        }
        // Do not attempt to kill hosts or save/close the user's projects automatically.
        if NSWorkspace.shared.runningApplications.contains(where: { $0.bundleIdentifier == "com.obsproject.obs-studio" }) {
            showFailure(SetupIssue(kind: "running", path: URL(fileURLWithPath: "/Applications/OBS.app"), detail: "OBS is running."))
            return
        }
        busy = true
        for button in [installButton, removeButton, guideButton] { button?.isEnabled = false }
        language.isEnabled = false
        window.standardWindowButton(.closeButton)?.isEnabled = false
        titleLabel.stringValue = t("処理中…", "Working…")
        body.stringValue = t("確認と処理が終わるまで、この画面を閉じないでください。", "Keep this window open until checks and changes finish.")
        progress.isHidden = false
        progress.startAnimation(nil)
        let core = SetupCore(home: fm.homeDirectoryForCurrentUser,
                             payload: Bundle.main.resourceURL!.appendingPathComponent("payload"),
                             verify: Self.verifySignature)
        DispatchQueue.global(qos: .userInitiated).async {
            let result = Result { try core.run(install: install) }
            DispatchQueue.main.async {
                self.busy = false
                for button in [self.installButton, self.removeButton, self.guideButton] { button?.isEnabled = true }
                self.language.isEnabled = true
                self.window.standardWindowButton(.closeButton)?.isEnabled = true
                self.progress.stopAnimation(nil)
                self.progress.isHidden = true
                switch result {
                case .success(let completion): self.showSuccess(install: install, warnings: completion.warnings)
                case .failure(let error):
                    self.showFailure(error as? SetupIssue ?? SetupIssue(kind: "operation", path: core.home, detail: error.localizedDescription))
                }
            }
        }
    }

    private static func verifySignature(_ url: URL) throws {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/codesign")
        process.arguments = ["--verify", "--deep", "--strict", url.path]
        // Drain while the process runs; no pipe-buffer deadlock on diagnostic output.
        let pipe = Pipe()
        process.standardError = pipe
        process.standardOutput = FileHandle.nullDevice
        try process.run()
        let diagnostics = pipe.fileHandleForReading.readDataToEndOfFile()
        process.waitUntilExit()
        guard process.terminationStatus == 0 else {
            throw NSError(domain: "CodeSignature", code: Int(process.terminationStatus),
                          userInfo: [NSLocalizedDescriptionKey: String(decoding: diagnostics, as: UTF8.self)])
        }
    }

    private func showSuccess(install: Bool, warnings: [String]) {
        titleLabel.stringValue = install ? t("インストール完了", "Installation complete") : t("アンインストール完了", "Uninstall complete")
        body.stringValue = install
            ? t("DAWとOBSを開き直してください。\n1. DAWのマスターの最後にDAS Sendを1個追加。\n2. OBSのソースにDAS Audio（DAW）を追加。\n3. DAWを再生し、OBSの音声ミキサーや録画などで音を確認。\n\nDAWに出ない場合は、プラグインを再スキャンしてください。",
                "Reopen your DAW and OBS.\n1. Insert one DAS Send at the end of the DAW master.\n2. Add DAS Audio (DAW) in OBS Sources.\n3. Play audio and check the OBS audio mixer or a recording.\n\nIf missing in your DAW, rescan plugins.")
            : t("DawAudioStreamerのプラグインを削除しました。\nDAWプロジェクト・OBSシーンは変更していません。\nSetupアプリは不要ならゴミ箱へ移動できます。",
                "DawAudioStreamer plugins have been removed.\nDAW projects and OBS scenes were not changed.\nYou can move the Setup app to Trash if no longer needed.")
        if !warnings.isEmpty {
            let alert = NSAlert()
            alert.alertStyle = .warning
            alert.messageText = t("処理は完了しましたが、後片付けが残っています", "Completed, but cleanup needs attention")
            alert.informativeText = t("一時ファイルを削除できませんでした。次の操作前にサポートへこの情報をお知らせください。\n", "Some temporary files could not be removed. Send these details to support before running setup again.\n") + warnings.joined(separator: "\n")
            alert.addButton(withTitle: "OK")
            alert.addButton(withTitle: t("詳細をコピー", "Copy details"))
            if alert.runModal() == .alertSecondButtonReturn { copy(warnings.joined(separator: "\n")) }
        }
    }

    private func showFailure(_ issue: SetupIssue) {
        titleLabel.stringValue = t("完了していません", "Setup did not complete")
        let advice: String
        switch issue.kind {
        case "permission":
            advice = t("インストール先に書き込めません。既存のプラグインは変更していません。Finderで表示し、［情報を見る］の［共有とアクセス権］を確認してください。変更できない場合はMacの管理者へ相談してください。",
                       "Cannot write to the destination. Existing plugins were not changed. Reveal it in Finder and check Get Info > Sharing & Permissions. Contact the Mac's administrator if you cannot change access.")
        case "payload":
            advice = t("必要なファイルが不足しているか、検証できません。公式サイトからZIPを再ダウンロードし、全体を展開してください。既存のプラグインは変更していません。",
                       "Required files are missing or could not be verified. Download the ZIP again from the official site and extract it fully. Existing plugins were not changed.")
        case "path":
            advice = t("インストール先が通常のフォルダではないため停止しました。リンクや既存ファイルは変更していません。サポートへご相談ください。",
                       "The destination is not a regular folder. Links and existing files were not changed. Contact support.")
        case "recovery":
            advice = t("復旧に必要なファイルが残っています。再実行や手動削除をせず、詳細をコピーしてサポートへお知らせください。",
                       "Recovery files remain. Do not retry or delete them manually. Copy the details and contact support.")
        case "busy": advice = t("別のセットアップを終了してから、もう一度お試しください。", "Close other copies of Setup and try again.")
        case "root": advice = t("通常のユーザーでFinderからSetupを開いてください。", "Open Setup in Finder as your normal user.")
        case "running": advice = t("OBSを終了してから、もう一度お試しください。", "Quit OBS and try again.")
        default:
            advice = t("ファイルの更新に失敗しました。変更したプラグインは処理前の状態へ戻しました。DAWとOBSを終了し、空き容量・アクセス権を確認して再試行してください。",
                       "Updating files failed. Changed plugins were restored to their previous state. Close your DAW and OBS, check free space and folder access, then retry.")
        }
        body.stringValue = advice
        let details = "\(issue.kind)\n\(issue.path.path)\n\(issue.detail)\n" + issue.recovery.map(\.path).joined(separator: "\n")
        let alert = NSAlert()
        alert.alertStyle = .warning
        alert.messageText = titleLabel.stringValue
        alert.informativeText = advice + "\n\n" + issue.path.path
        alert.addButton(withTitle: t("閉じる", "Close"))
        alert.addButton(withTitle: t("Finderで表示", "Show in Finder"))
        alert.addButton(withTitle: t("詳細をコピー", "Copy details"))
        switch alert.runModal() {
        case .alertSecondButtonReturn:
            var folder = issue.path
            while !fm.fileExists(atPath: folder.path) && folder.path != "/" { folder.deleteLastPathComponent() }
            NSWorkspace.shared.activateFileViewerSelecting([folder])
        case .alertThirdButtonReturn: copy(details)
        default: break
        }
    }
    private func copy(_ text: String) {
        NSPasteboard.general.clearContents()
        NSPasteboard.general.setString(text, forType: .string)
    }
    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply { busy ? .terminateCancel : .terminateNow }
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }
}
