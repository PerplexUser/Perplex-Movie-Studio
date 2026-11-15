#!/usr/bin/env python3
"""Assemble, relocate, audit and sign a native macOS application bundle."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
MAGICS = {b"\xfe\xed\xfa\xce", b"\xce\xfa\xed\xfe", b"\xfe\xed\xfa\xcf", b"\xcf\xfa\xed\xfe",
          b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca", b"\xca\xfe\xba\xbf", b"\xbf\xba\xfe\xca"}
SYSTEM = ("/System/Library/", "/usr/lib/")

def run(*args):
    subprocess.run([str(a) for a in args], check=True)

def output(*args):
    return subprocess.check_output([str(a) for a in args], text=True)

def machos(root):
    found = []
    for path in sorted(root.rglob("*")):
        if path.is_file() and not path.is_symlink():
            with path.open("rb") as stream:
                if stream.read(4) in MAGICS:
                    found.append(path)
    return found

def macho_hashes(root):
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in machos(root)}

def imports(path):
    return [line.strip().split(" (compatibility version", 1)[0]
            for line in output("otool", "-L", path).splitlines()[1:] if " (compatibility version" in line]

def install_id(path):
    lines = output("otool", "-D", path).splitlines()
    return lines[1].strip() if len(lines) > 1 else None

def rpaths(path):
    return re.findall(r"cmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset", output("otool", "-l", path))

def canonical_dependency(dep, frameworks):
    if dep.startswith(SYSTEM):
        return dep
    match = re.search(r"(Qt[^/]+\.framework/(?:Versions/[^/]+/)?[^/]+)$", dep)
    relative = match.group(1) if match else Path(dep).name
    target = frameworks / relative
    if not target.is_file() or not target.resolve().is_relative_to(frameworks.resolve()):
        raise RuntimeError(f"Unbundled dependency: {dep}")
    return "@rpath/" + relative

def relocate_and_sign(app, arch, identity="-"):
    frameworks = app / "Contents/Frameworks"
    binaries = machos(app)
    if not binaries:
        raise RuntimeError("No Mach-O executables in application")
    for binary in binaries:
        arches = output("lipo", "-archs", binary).strip().split()
        if arch not in arches:
            raise RuntimeError(f"Missing {arch}: {binary}")
        if len(arches) > 1:
            temporary = binary.with_name(binary.name + ".thin")
            run("lipo", binary, "-thin", arch, "-output", temporary)
            shutil.copymode(binary, temporary)
            temporary.replace(binary)
        # Keep the existing signature allocation while relocating. Removing it
        # can leave Qt's __LINKEDIT padding inconsistent for install_name_tool.
        # The final codesign --force pass replaces the invalidated signature.
        old_id = install_id(binary)
        for dep in imports(binary):
            if dep == old_id:
                continue
            new = canonical_dependency(dep, frameworks)
            if new != dep:
                run("install_name_tool", "-change", dep, new, binary)
        if old_id:
            new_id = ("@rpath/" + str(binary.relative_to(frameworks))) if binary.is_relative_to(frameworks) else ("@loader_path/" + binary.name)
            if new_id != old_id:
                run("install_name_tool", "-id", new_id, binary)
        for old in set(rpaths(binary)):
            run("install_name_tool", "-delete_rpath", old, binary)
        relative = os.path.relpath(frameworks, binary.parent)
        run("install_name_tool", "-add_rpath", "@loader_path/" + relative, binary)
    report = []
    for binary in binaries:
        own = install_id(binary)
        deps = [d for d in imports(binary) if d != own]
        for dep in deps:
            if canonical_dependency(dep, frameworks) != dep:
                raise RuntimeError(f"Nonportable dependency after relocation: {dep}")
        commands = output("otool", "-l", binary)
        minimums = re.findall(r"\bminos (\d+\.\d+(?:\.\d+)?)", commands)
        minimums += re.findall(r"cmd LC_VERSION_MIN_MACOSX\s+cmdsize \d+\s+version (\d+\.\d+(?:\.\d+)?)", commands)
        if not minimums or any(tuple(map(int, v.split(".")[:2])) > (12, 0) for v in minimums):
            raise RuntimeError(f"Unexpected minimum macOS version in {binary}: {minimums}")
        report.append({"file": str(binary.relative_to(app)), "dependencies": deps,
                       "rpaths": rpaths(binary), "minimum_macos": minimums})
    signing = ["--timestamp=none"] if identity == "-" else ["--options", "runtime", "--timestamp"]
    main_executable = app / "Contents/MacOS/PerplexMovieStudio"
    for binary in sorted(binaries, key=lambda p: len(p.parts), reverse=True):
        # Signing the main executable also seals the app. Finish nested code first.
        if binary == main_executable:
            continue
        run("codesign", "--force", "--sign", identity, *signing, binary)
    for framework in sorted(frameworks.glob("*.framework")):
        run("codesign", "--force", "--sign", identity, *signing, framework)
    run("codesign", "--force", "--sign", identity, *signing, app)
    run("codesign", "--verify", "--deep", "--strict", "--verbose=2", app)
    return report

def copy_runtime(prefix, frameworks, clean=False):
    frameworks.mkdir(parents=True, exist_ok=True)
    if clean:
        # Never leave a second unreviewed SDK copy of FFmpeg in the bundle.
        for path in frameworks.glob("*.dylib"):
            if re.match(r"lib(?:avcodec|avformat|avutil|avfilter|avdevice|swscale|swresample|postproc|mp3lame)(?:\.|-)", path.name):
                path.unlink()
    copied = set()
    for library in sorted((prefix / "lib").glob("*.dylib")):
        name = Path(install_id(library) or library.name).name
        if name not in copied:
            shutil.copy2(library.resolve(), frameworks / name)
            copied.add(name)
    if not (frameworks / "libavutil.59.dylib").is_file():
        raise RuntimeError("Reviewed FFmpeg 7 runtime is missing")

def assemble(args):
    if args.output.exists():
        raise RuntimeError("Use a fresh package output directory")
    args.output.mkdir(parents=True)
    app = args.output / "Perplex Movie Studio.app"
    shutil.copytree(args.build / "PerplexMovieStudio.app", app, symlinks=True)
    contents = app / "Contents"
    frameworks, resources = contents / "Frameworks", contents / "Resources"
    media = contents / "MacOS/media"
    media.mkdir()
    extras = []
    for name in ("ffmpeg", "ffprobe"):
        target = media / name
        shutil.copy2(args.ffmpeg / "bin" / name, target)
        extras.append(target)
    copy_runtime(args.ffmpeg, frameworks)
    for plugin in ("platforms/libqcocoa.dylib", "platforms/libqoffscreen.dylib",
                   "multimedia/libffmpegmediaplugin.dylib", "imageformats/libqjpeg.dylib"):
        target = contents / "PlugIns" / plugin
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(args.qt / "plugins" / plugin, target)
        extras.append(target)
    run(args.qt / "bin/macdeployqt", app, "-always-overwrite", "-no-strip", "-no-plugins",
        "-libpath=" + str(args.ffmpeg / "lib"), *("-executable=" + str(p) for p in extras))
    copy_runtime(args.ffmpeg, frameworks, clean=True)
    (resources / "qt.conf").write_text("[Paths]\nPrefix=.\nLibraries=Frameworks\nPlugins=PlugIns\nTranslations=Resources/translations\n")
    (resources / "translations").mkdir(exist_ok=True)
    shutil.copy2(args.qt / "translations/qtbase_de.qm", resources / "translations")
    for name in ("LICENSE", "THIRD_PARTY_NOTICES.md", "README.md", "CHANGELOG.md"):
        shutil.copy2(ROOT / name, args.output / name)
        shutil.copy2(ROOT / name, resources / name)
    shutil.copytree(ROOT / "third-party/licenses", resources / "licenses")
    shutil.copytree(ROOT / "docs", resources / "docs", ignore=shutil.ignore_patterns("specification"))
    shutil.copytree(ROOT / "examples", resources / "examples")
    shutil.copy2(ROOT / "docs/MACOS_DE.md", args.output / "MACOS-START.md")
    report = relocate_and_sign(app, args.arch, args.sign_identity)
    (args.output / "dependency-check.json").write_text(json.dumps({
        "architecture": args.arch, "minimum_macos": "12.0", "notarized": False,
        "signing": "ad-hoc" if args.sign_identity == "-" else "Developer ID (not notarized)",
        "binaries": report}, indent=2) + "\n")
    (args.output / "Applications").symlink_to("/Applications")

def archive(args):
    verification = json.loads(args.verification.read_text())
    app = args.output / "Perplex Movie Studio.app"
    if not verification.get("passed") or verification.get("platform") != "Darwin" or verification.get("architecture") != args.arch:
        raise RuntimeError("A passing native macOS verification report is required")
    if verification.get("release_macho_sha256") != macho_hashes(app):
        raise RuntimeError("Application binaries changed after verification")
    shutil.copy2(args.verification, args.output / "verification.json")
    sums = []
    for path in sorted(args.output.rglob("*")):
        if path.is_file() and not path.is_symlink() and path.name != "SHA256SUMS":
            sums.append(hashlib.sha256(path.read_bytes()).hexdigest() + "  " + str(path.relative_to(args.output)))
    (args.output / "SHA256SUMS").write_text("\n".join(sums) + "\n")
    zip_path, dmg_path = Path(str(args.output) + ".zip"), Path(str(args.output) + ".dmg")
    if zip_path.exists() or dmg_path.exists():
        raise RuntimeError("Package archives already exist")
    run("ditto", "-c", "-k", "--sequesterRsrc", "--keepParent", args.output, zip_path)
    run("hdiutil", "create", "-fs", "HFS+", "-srcfolder", args.output, "-volname", "Perplex Movie Studio", "-format", "UDZO", dmg_path)
    run("hdiutil", "verify", dmg_path)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("assemble", "archive"))
    parser.add_argument("--arch", choices=("arm64", "x86_64"), required=True)
    for arg in ("build", "qt", "ffmpeg", "output", "verification"):
        parser.add_argument("--" + arg, type=lambda p: Path(p).resolve())
    parser.add_argument("--sign-identity", default="-")
    args = parser.parse_args()
    if platform.system() != "Darwin" or platform.machine() != args.arch:
        raise SystemExit("Run on a native macOS host matching --arch")
    (assemble if args.mode == "assemble" else archive)(args)

if __name__ == "__main__":
    main()
