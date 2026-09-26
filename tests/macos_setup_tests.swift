// SPDX-License-Identifier: AGPL-3.0-only
// Never points SetupCore at the real user's Library. Each fixture owns one temporary tree.
import Foundation
import Darwin

private let fm = FileManager.default
private enum TestFailure: Error { case failed(String) }
private func expect(_ condition: @autoclosure () throws -> Bool, _ message: String) throws {
    if try !condition() { throw TestFailure.failed(message) }
}

private final class Fixture {
    let root: URL
    let home: URL
    let payload: URL
    init() throws {
        root = fm.temporaryDirectory.appendingPathComponent("das-tests-" + UUID().uuidString)
        home = root.appendingPathComponent("test-user")
        payload = root.appendingPathComponent("payload")
        try fm.createDirectory(at: home, withIntermediateDirectories: true)
        for item in SetupCore.components {
            let bundle = payload.appendingPathComponent(item.name)
            let contents = bundle.appendingPathComponent("Contents")
            try fm.createDirectory(at: contents.appendingPathComponent("MacOS"), withIntermediateDirectories: true)
            let plist = try PropertyListSerialization.data(fromPropertyList: ["CFBundleExecutable": item.executable], format: .xml, options: 0)
            try plist.write(to: contents.appendingPathComponent("Info.plist"))
            let executable = contents.appendingPathComponent("MacOS/" + item.executable)
            try Data("fixture".utf8).write(to: executable)
            // Only synthetic test files; never changes installed folders or user permissions.
            try fm.setAttributes([.posixPermissions: 0o755], ofItemAtPath: executable.path)
        }
    }
    func core(_ hook: @escaping (String, Int) throws -> Void = { _, _ in }) -> SetupCore {
        SetupCore(home: home, payload: payload, verify: { _ in }, checkpoint: hook)
    }
    func marker(_ item: SetupCore.Component) -> URL { core().target(item).appendingPathComponent("old-version") }
    func seed() throws {
        _ = try core().run(install: true)
        for item in SetupCore.components { try Data("keep".utf8).write(to: marker(item)) }
    }
    func expectOld() throws {
        for item in SetupCore.components { try expect(fm.fileExists(atPath: marker(item).path), "Existing version lost: \(item.name)") }
    }
    func cleanup() throws {
        // root is created by this fixture; never a user-selected or environment-provided path.
        try expect(root.lastPathComponent.hasPrefix("das-tests-"), "Unsafe test cleanup")
        try fm.removeItem(at: root)
    }
}

@main
private struct SetupTests {
    static func fails(_ core: SetupCore, install: Bool = true, kind: String? = nil) throws -> SetupIssue {
        do { _ = try core.run(install: install) }
        catch let issue as SetupIssue {
            if let kind { try expect(issue.kind == kind, "Expected \(kind), got \(issue.kind)") }
            return issue
        }
        throw TestFailure.failed("Expected failure")
    }
    static func test(_ name: String, _ body: (Fixture) throws -> Void) throws {
        let f = try Fixture()
        do { try body(f); try f.cleanup() }
        catch { print("FAIL \(name); fixture retained at \(f.root.path)"); throw error }
        print("PASS \(name)")
    }
    static func main() throws {
        try test("install, reinstall, uninstall, repeat uninstall; preserve unrelated files") { f in
            let unrelated = f.home.appendingPathComponent("project.txt")
            try Data("project".utf8).write(to: unrelated)
            try f.seed()
            let sibling = f.core().target(SetupCore.components[0]).deletingLastPathComponent().appendingPathComponent("Other.vst3")
            try fm.createDirectory(at: sibling, withIntermediateDirectories: false)
            _ = try f.core().run(install: true)
            for item in SetupCore.components { try expect(!fm.fileExists(atPath: f.marker(item).path), "Old marker was not replaced") }
            _ = try f.core().run(install: false)
            _ = try f.core().run(install: false)
            for item in SetupCore.components { try expect(!(try f.core().exists(f.core().target(item))), "Plugin remains") }
            try expect(fm.fileExists(atPath: unrelated.path) && fm.fileExists(atPath: sibling.path), "Unrelated file removed")
        }
        try test("missing payload does not alter installed version") { f in
            try f.seed()
            try fm.removeItem(at: f.payload.appendingPathComponent(SetupCore.components[2].name))
            _ = try fails(f.core(), kind: "payload")
            try f.expectOld()
            // Uninstall must work even when the download payload is missing.
            _ = try f.core().run(install: false)
        }
        try test("invalid executable and invalid signature rejected") { f in
            try f.seed()
            let invalid = SetupCore(home: f.home, payload: f.payload, verify: { _ in throw TestFailure.failed("Bad signature") })
            _ = try fails(invalid, kind: "payload")
            let item = SetupCore.components[0]
            try fm.removeItem(at: f.payload.appendingPathComponent(item.name + "/Contents/MacOS/" + item.executable))
            _ = try fails(f.core(), kind: "payload")
            try f.expectOld()
        }
        for stage in ["copy", "commit", "verify"] {
            try test("\(stage) failure rolls back all components") { f in
                try f.seed()
                _ = try fails(f.core { phase, index in
                    if phase == stage && index == 1 { throw TestFailure.failed("Injected \(stage)") }
                }, kind: "operation")
                try f.expectOld()
                _ = try f.core().run(install: true)
            }
        }
        try test("fresh install rollback leaves no plugins") { f in
            _ = try fails(f.core { phase, index in
                if phase == "commit" && index == 2 { throw TestFailure.failed("Injected") }
            })
            for item in SetupCore.components { try expect(!(try f.core().exists(f.core().target(item))), "Partial install remains") }
        }
        try test("uninstall failure restores plugins") { f in
            try f.seed()
            _ = try fails(f.core { phase, index in
                if phase == "commit" && index == 1 { throw TestFailure.failed("Injected") }
            }, install: false, kind: "operation")
            try f.expectOld()
        }
        try test("recovery failure is explicit and prevents another overwrite") { f in
            try f.seed()
            let issue = try fails(f.core { phase, index in
                if (phase == "commit" && index == 1) || (phase == "rollback" && index == 0) { throw TestFailure.failed("Injected") }
            }, kind: "recovery")
            try expect(!issue.recovery.isEmpty, "Missing recovery paths")
            _ = try fails(f.core(), kind: "recovery")
        }
        try test("cleanup warning is not silent or mislabeled as install failure") { f in
            let result = try f.core { phase, _ in
                if phase == "cleanup" { throw TestFailure.failed("Injected") }
            }.run(install: true)
            try expect(result.warnings.count == 3, "Missing cleanup warnings")
            for item in SetupCore.components { try expect(try f.core().exists(f.core().target(item)), "Install incomplete") }
        }
        try test("read-only target parent detected before deleting old version") { f in
            try expect(geteuid() != 0, "Run tests without root so permission checks are meaningful")
            try f.seed()
            let parent = f.core().target(SetupCore.components[0]).deletingLastPathComponent()
            try fm.setAttributes([.posixPermissions: 0o555], ofItemAtPath: parent.path)
            do {
                _ = try fails(f.core(), kind: "permission")
                _ = try fails(f.core(), install: false, kind: "permission")
                try f.expectOld()
            } catch {
                try fm.setAttributes([.posixPermissions: 0o755], ofItemAtPath: parent.path)
                throw error
            }
            try fm.setAttributes([.posixPermissions: 0o755], ofItemAtPath: parent.path)
        }
        try test("inaccessible is not treated as absent during uninstall") { f in
            try f.seed()
            let parent = f.core().target(SetupCore.components[0]).deletingLastPathComponent()
            try fm.setAttributes([.posixPermissions: 0o000], ofItemAtPath: parent.path)
            do {
                _ = try fails(f.core(), install: false, kind: "permission")
            } catch {
                try fm.setAttributes([.posixPermissions: 0o755], ofItemAtPath: parent.path)
                throw error
            }
            try fm.setAttributes([.posixPermissions: 0o755], ofItemAtPath: parent.path)
            try f.expectOld()
        }
        try test("destination symlink is not followed") { f in
            let link = f.home.appendingPathComponent("Library")
            let outside = f.root.appendingPathComponent("outside")
            try fm.createDirectory(at: outside, withIntermediateDirectories: false)
            try fm.createSymbolicLink(at: link, withDestinationURL: outside)
            _ = try fails(f.core(), kind: "path")
            let outsideContents = try fm.contentsOfDirectory(atPath: outside.path)
            try expect(outsideContents.isEmpty, "Wrote through a symlink")
        }
        try test("payload symlink cannot read outside its bundle") { f in
            let outside = f.root.appendingPathComponent("private.txt")
            try Data("private".utf8).write(to: outside)
            let link = f.payload.appendingPathComponent(SetupCore.components[0].name + "/external")
            try fm.createSymbolicLink(at: link, withDestinationURL: outside)
            _ = try fails(f.core(), kind: "payload")
        }
        try test("simultaneous setup is refused") { f in
            let fd = open(fm.temporaryDirectory.appendingPathComponent("org.dawaudiostreamer.setup-\(getuid()).lock").path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)
            guard fd >= 0 else { throw TestFailure.failed("Cannot create lock") }
            defer { close(fd) }
            try expect(flock(fd, LOCK_EX | LOCK_NB) == 0, "Cannot lock")
            _ = try fails(f.core(), kind: "busy")
        }
        // CI additionally exercises the actual, signed release payload in an isolated home.
        if CommandLine.arguments.count == 2 {
            try test("packaged bundles install and uninstall") { f in
                let payload = URL(fileURLWithPath: CommandLine.arguments[1])
                let core = SetupCore(home: f.home, payload: payload, verify: { bundle in
                    let p = Process()
                    p.executableURL = URL(fileURLWithPath: "/usr/bin/codesign")
                    p.arguments = ["--verify", "--deep", "--strict", bundle.path]
                    try p.run(); p.waitUntilExit()
                    try expect(p.terminationStatus == 0, "Signature verification failed")
                })
                _ = try core.run(install: true)
                _ = try core.run(install: true)
                _ = try core.run(install: false)
            }
        }
    }
}
