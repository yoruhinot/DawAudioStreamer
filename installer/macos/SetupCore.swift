// SPDX-License-Identifier: AGPL-3.0-only
import Foundation
import Darwin

struct SetupIssue: Error {
    let kind: String
    let path: URL
    let detail: String
    var recovery: [URL] = []
}

struct SetupResult {
    // A completed operation with cleanup warnings is not a failed installation.
    let warnings: [String]
}

final class SetupCore {
    struct Component {
        let name: String
        let directory: String
        let executable: String
    }
    static let components = [
        Component(name: "DAS Send.vst3", directory: "Library/Audio/Plug-Ins/VST3", executable: "DAS Send"),
        Component(name: "DAS Send.component", directory: "Library/Audio/Plug-Ins/Components", executable: "DAS Send"),
        Component(name: "das-obs-source.plugin", directory: "Library/Application Support/obs-studio/plugins", executable: "das-obs-source")
    ]
    private let fm = FileManager.default
    let home: URL
    let payload: URL
    // Injection is only through the test harness, never environment variables or CLI arguments.
    let verify: (URL) throws -> Void
    let checkpoint: (String, Int) throws -> Void

    init(home: URL, payload: URL,
         verify: @escaping (URL) throws -> Void,
         checkpoint: @escaping (String, Int) throws -> Void = { _, _ in }) {
        self.home = home.resolvingSymlinksInPath().standardizedFileURL
        self.payload = payload
        self.verify = verify
        self.checkpoint = checkpoint
    }

    func exists(_ url: URL) throws -> Bool {
        // Unlike fileExists, this distinguishes inaccessible from absent (especially on uninstall).
        // attributesOfItem also sees a dangling symlink; do not silently follow it.
        do {
            _ = try fm.attributesOfItem(atPath: url.path)
            return true
        } catch {
            let problem = error as NSError
            if problem.domain == NSCocoaErrorDomain &&
                [NSFileNoSuchFileError, NSFileReadNoSuchFileError].contains(problem.code) { return false }
            throw error
        }
    }

    func target(_ item: Component) -> URL {
        home.appendingPathComponent(item.directory).appendingPathComponent(item.name)
    }

    private func checkPath(_ url: URL) throws {
        guard url.path.hasPrefix(home.path + "/") else {
            throw SetupIssue(kind: "path", path: url, detail: "Destination is outside the current user's home.")
        }
        var cursor = url
        while cursor.path != home.path {
            if try exists(cursor) {
                let attrs = try fm.attributesOfItem(atPath: cursor.path)
                guard attrs[.type] as? FileAttributeType == .typeDirectory else {
                    throw SetupIssue(kind: "path", path: cursor, detail: "Expected a folder, not a file or symbolic link.")
                }
            }
            cursor.deleteLastPathComponent()
        }
    }

    private func validate(_ bundle: URL, item: Component) throws {
        do {
            let attrs = try fm.attributesOfItem(atPath: bundle.path)
            guard attrs[.type] as? FileAttributeType == .typeDirectory else {
                throw NSError(domain: "Setup", code: 1, userInfo: [NSLocalizedDescriptionKey: "Missing bundle folder"])
            }
            let plistURL = bundle.appendingPathComponent("Contents/Info.plist")
            let plist = try PropertyListSerialization.propertyList(from: Data(contentsOf: plistURL), format: nil) as? [String: Any]
            guard plist?["CFBundleExecutable"] as? String == item.executable,
                  fm.isExecutableFile(atPath: bundle.appendingPathComponent("Contents/MacOS/" + item.executable).path) else {
                throw NSError(domain: "Setup", code: 2, userInfo: [NSLocalizedDescriptionKey: "Missing or invalid bundle executable"])
            }
            // Legitimate bundle-internal links are allowed, but never links outside the bundle.
            guard let enumerator = fm.enumerator(at: bundle, includingPropertiesForKeys: [.isSymbolicLinkKey]) else {
                throw NSError(domain: "Setup", code: 3, userInfo: [NSLocalizedDescriptionKey: "Cannot read bundle"])
            }
            for case let entry as URL in enumerator {
                if try entry.resourceValues(forKeys: [.isSymbolicLinkKey]).isSymbolicLink == true {
                    guard entry.resolvingSymlinksInPath().path.hasPrefix(bundle.resolvingSymlinksInPath().path + "/") else {
                        throw SetupIssue(kind: "payload", path: entry, detail: "Bundle contains an external symbolic link.")
                    }
                }
            }
            try verify(bundle)
        } catch {
            throw SetupIssue(kind: "payload", path: bundle, detail: String(describing: error))
        }
    }

    private func probe(_ parent: URL) throws {
        let scratch = parent.appendingPathComponent(".das-probe-" + UUID().uuidString)
        let moved = scratch.appendingPathExtension("moved")
        do {
            try fm.createDirectory(at: scratch, withIntermediateDirectories: false)
            try Data("DAS".utf8).write(to: scratch.appendingPathComponent("probe"))
            try fm.moveItem(at: scratch, to: moved)
            try fm.removeItem(at: moved)
        } catch {
            var cleanup = [URL]()
            for url in [scratch, moved] {
                do {
                    if try exists(url) { try fm.removeItem(at: url) }
                } catch { cleanup.append(url) }
            }
            throw SetupIssue(kind: "permission", path: parent, detail: error.localizedDescription, recovery: cleanup)
        }
    }

    private struct Work {
        let target: URL
        let folder: URL
        var oldMoved = false
        var newMoved = false
        var previous: URL { folder.appendingPathComponent("previous") }
        var staged: URL { folder.appendingPathComponent("new-" + target.lastPathComponent) }
    }

    func run(install: Bool) throws -> SetupResult {
        // A per-login temporary directory lock serializes separate copies of Setup.app.
        let lockPath = fm.temporaryDirectory.appendingPathComponent("org.dawaudiostreamer.setup-\(getuid()).lock").path
        let fd = open(lockPath, O_CREAT | O_RDWR | O_NOFOLLOW, S_IRUSR | S_IWUSR)
        guard fd >= 0 else { throw SetupIssue(kind: "busy", path: home, detail: "Cannot open setup lock.") }
        defer { close(fd) }
        guard flock(fd, LOCK_EX | LOCK_NB) == 0 else {
            throw SetupIssue(kind: "busy", path: home, detail: "Another setup is running.")
        }
        defer { flock(fd, LOCK_UN) }

        if install {
            for item in Self.components { try validate(payload.appendingPathComponent(item.name), item: item) }
        }
        // All existing paths are checked before changing any installed bundle.
        for item in Self.components {
            let destination = target(item)
            do {
                try checkPath(destination)
                var parent = destination.deletingLastPathComponent()
                if try exists(parent) {
                    let leftovers = try fm.contentsOfDirectory(at: parent, includingPropertiesForKeys: nil)
                        .filter { $0.lastPathComponent.hasPrefix(".das-setup-") }
                    if !leftovers.isEmpty {
                        throw SetupIssue(kind: "recovery", path: parent, detail: "An earlier setup left recovery files. Keep them until recovery is resolved.", recovery: leftovers)
                    }
                }
                if !install, try !exists(destination) { continue }
                while try !exists(parent) { parent.deleteLastPathComponent() }
                try probe(parent)
            } catch let issue as SetupIssue { throw issue }
            catch { throw SetupIssue(kind: "permission", path: destination.deletingLastPathComponent(), detail: error.localizedDescription) }
        }

        var work = [Work]()
        var current = home
        do {
            for (index, item) in Self.components.enumerated() {
                let destination = target(item)
                if !install, try !exists(destination) { continue }
                current = destination
                let parent = destination.deletingLastPathComponent()
                try fm.createDirectory(at: parent, withIntermediateDirectories: true)
                try probe(parent)
                let folder = parent.appendingPathComponent(".das-setup-" + UUID().uuidString)
                try fm.createDirectory(at: folder, withIntermediateDirectories: false)
                let entry = Work(target: destination, folder: folder)
                work.append(entry)
                try Data(destination.path.utf8).write(to: folder.appendingPathComponent("destination.txt"))
                if install {
                    try checkpoint("copy", index)
                    try fm.copyItem(at: payload.appendingPathComponent(item.name), to: entry.staged)
                    try validate(entry.staged, item: item)
                }
            }
            // Only after every copy validates do we touch the existing version.
            for index in work.indices {
                current = work[index].target
                try checkPath(current)
                try checkpoint("commit", index)
                if try exists(current) {
                    try fm.moveItem(at: current, to: work[index].previous)
                    work[index].oldMoved = true
                }
                if install {
                    try fm.moveItem(at: work[index].staged, to: current)
                    work[index].newMoved = true
                }
            }
            for (index, item) in Self.components.enumerated() {
                current = target(item)
                try checkpoint("verify", index)
                if install { try validate(current, item: item) }
                else if try exists(current) {
                    throw SetupIssue(kind: "operation", path: current, detail: "The plugin still exists.")
                }
            }
        } catch {
            let original = error
            var recovery = [URL]()
            for index in work.indices.reversed() {
                let entry = work[index]
                do {
                    try checkpoint("rollback", index)
                    if entry.newMoved { try fm.moveItem(at: entry.target, to: entry.staged) }
                    if entry.oldMoved { try fm.moveItem(at: entry.previous, to: entry.target) }
                    try fm.removeItem(at: entry.folder)
                } catch { recovery.append(entry.folder) }
            }
            throw SetupIssue(kind: recovery.isEmpty ? "operation" : "recovery", path: current,
                             detail: String(describing: original), recovery: recovery)
        }
        var warnings = [String]()
        for (index, entry) in work.enumerated() {
            do {
                try checkpoint("cleanup", index)
                try fm.removeItem(at: entry.folder)
            } catch { warnings.append("\(entry.folder.path): \(error.localizedDescription)") }
        }
        return SetupResult(warnings: warnings)
    }
}
