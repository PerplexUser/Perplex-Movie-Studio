#!/usr/bin/env python3
"""Assemble a private, replaceable Qt/FFmpeg runtime and package verified binaries."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[1]
DEPENDENCIES = "libc6 (>= 2.35), libstdc++6 (>= 12), libgcc-s1, libatomic1, libzstd1, zlib1g, libglib2.0-0, libdbus-1-3, libfontconfig1, libfreetype6, libgl1, libegl1, libopengl0, libx11-6, libx11-xcb1, libxext6, libxrandr2, libxfixes3, libxi6, libxrender1, libxcb1, libxcb-cursor0, libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-randr0, libxcb-render0, libxcb-render-util0, libxcb-shape0, libxcb-shm0, libxcb-sync1, libxcb-xfixes0, libxcb-xinerama0, libxcb-xkb1, libxkbcommon0, libxkbcommon-x11-0, libpulse0, libasound2, fonts-dejavu-core"

def elf_files(root):
    result = []
    for p in root.rglob("*"):
        if p.is_file():
            with p.open("rb") as stream:
                if stream.read(4) == b"\x7fELF":
                    result.append(p)
    return sorted(result)

def hashes(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in elf_files(root)}

def patch(path, relative_lib):
    subprocess.run(["patchelf", "--set-rpath", relative_lib, str(path)], check=True)

def assemble(a):
    out = a.output
    out.mkdir(parents=True, exist_ok=False)
    for folder in ("bin", "lib", "translations"):
        (out / folder).mkdir()
    for name, origin in (("PerplexMovieStudio", a.build), ("ffmpeg", a.ffmpeg / "bin"), ("ffprobe", a.ffmpeg / "bin")):
        shutil.copy2(origin / name, out / "bin" / name)
    for plugin in ("platforms/libqxcb.so", "platforms/libqoffscreen.so", "multimedia/libffmpegmediaplugin.so", "imageformats/libqjpeg.so"):
        target = out / "plugins" / plugin
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(a.qt / "plugins" / plugin, target)
    available = {p.name: p for p in (a.qt / "lib").glob("*.so*") if p.name.startswith(("libQt6", "libicu"))}
    available.update({p.name: p for p in (a.ffmpeg / "lib").glob("*.so*")})
    pending = elf_files(out)
    seen = set()
    imports = {}
    while pending:
        binary = pending.pop()
        if binary in seen:
            continue
        seen.add(binary)
        header = subprocess.check_output(["readelf", "-h", str(binary)], text=True)
        if "ELF64" not in header or "Advanced Micro Devices X86-64" not in header:
            raise RuntimeError("Expected Linux x86_64 ELF: " + str(binary))
        dynamic = subprocess.check_output(["readelf", "-d", str(binary)], text=True)
        needed = re.findall(r"\(NEEDED\).*?\[(.*?)\]", dynamic)
        imports[binary.relative_to(out).as_posix()] = needed
        for name in needed:
            if name in available:
                target = out / "lib" / name
                if not target.exists():
                    shutil.copy2(available[name], target)
                    pending.append(target)
            elif name.startswith(("libQt6", "libicu", "libav", "libsw", "libmp3lame")):
                raise RuntimeError("Missing private runtime dependency: " + name)
    for binary in elf_files(out):
        relative = os.path.relpath(out / "lib", binary.parent)
        patch(binary, "$ORIGIN" if relative == "." else "$ORIGIN/" + relative)
    env = dict(os.environ, LD_LIBRARY_PATH=str(out / "lib"))
    for binary in elf_files(out):
        result = subprocess.run(["ldd", "-r", str(binary)], env=env, capture_output=True, text=True, check=True)
        if "not found" in result.stdout or "undefined symbol" in result.stdout:
            raise RuntimeError(result.stdout)
    shutil.copy2(a.qt / "translations/qtbase_de.qm", out / "translations")
    (out / "bin/qt.conf").write_text("[Paths]\nPrefix=..\nPlugins=plugins\nTranslations=translations\n")
    launcher = out / "PerplexMovieStudio"
    launcher.write_text('''#!/bin/sh
app_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export LD_LIBRARY_PATH="$app_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$app_dir/plugins"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
exec "$app_dir/bin/PerplexMovieStudio" "$@"
''')
    launcher.chmod(0o755)
    for name in ("LICENSE", "THIRD_PARTY_NOTICES.md", "README.md", "CHANGELOG.md"):
        shutil.copy2(ROOT / name, out)
    shutil.copytree(ROOT / "docs", out / "docs", ignore=shutil.ignore_patterns("specification"))
    shutil.copytree(ROOT / "third-party/licenses", out / "licenses")
    shutil.copytree(ROOT / "examples", out / "examples")
    ico = (ROOT / "assets/perplex.ico").read_bytes()
    count = struct.unpack_from("<H", ico, 4)[0]
    for i in range(count):
        width, height, _, _, _, _, size, offset = struct.unpack_from("<BBBBHHII", ico, 6 + 16 * i)
        if (width or 256) == 256:
            (out / "perplex-movie-studio.png").write_bytes(ico[offset:offset + size])
            break
    if not (out / "perplex-movie-studio.png").exists():
        raise RuntimeError("Missing original 256px icon")
    (out / "dependency-check.json").write_text(json.dumps({"architecture": "x86_64", "missing": [], "imports": imports}, indent=2) + "\n")

def archive(a):
    out = a.output
    report = json.loads(a.verification.read_text())
    if not report.get("passed") or report["release_sha256"] != hashes(out):
        raise RuntimeError("The exact package must pass native verification before archiving")
    shutil.copy2(a.verification, out / "verification.json")
    (out / "SHA256SUMS.txt").write_text("\n".join(
        f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(out).as_posix()}"
        for p in sorted(out.rglob("*")) if p.is_file() and p.name != "SHA256SUMS.txt") + "\n")
    version = (ROOT / "VERSION").read_text().strip()
    debroot = ROOT / "build/deb-root"
    debroot.mkdir(parents=True, exist_ok=False)
    installed = debroot / "opt/perplex-movie-studio"
    shutil.copytree(out, installed)
    for folder in ("DEBIAN", "usr/bin", "usr/share/applications", "usr/share/icons/hicolor/256x256/apps", "usr/share/mime/packages"):
        (debroot / folder).mkdir(parents=True, exist_ok=True)
    (debroot / "DEBIAN/control").write_text(
        f"Package: perplex-movie-studio\nVersion: {version}\nSection: video\nPriority: optional\nArchitecture: amd64\n"
        f"Maintainer: Michael Lappenbusch\nHomepage: https://www.perplex.click\nDepends: {DEPENDENCIES}\n"
        "Description: Local video editor with titles, transitions and layered opacity\n"
        " Edit videos, images and audio on a timeline and export landscape or portrait films.\n")
    wrapper = debroot / "usr/bin/perplex-movie-studio"
    wrapper.write_text('#!/bin/sh\nexec /opt/perplex-movie-studio/PerplexMovieStudio "$@"\n')
    wrapper.chmod(0o755)
    (debroot / "usr/share/applications/perplex-movie-studio.desktop").write_text(
        "[Desktop Entry]\nType=Application\nName=Perplex Movie Studio\nComment=Edit videos, images and titles\n"
        "Comment[de]=Videos, Bilder und Texte bearbeiten\nExec=perplex-movie-studio %f\nIcon=perplex-movie-studio\n"
        "Terminal=false\nCategories=AudioVideo;Video;AudioVideoEditing;\nMimeType=application/x-perplex-movie-studio;\n")
    shutil.copy2(out / "perplex-movie-studio.png", debroot / "usr/share/icons/hicolor/256x256/apps")
    (debroot / "usr/share/mime/packages/perplex-movie-studio.xml").write_text(
        '<?xml version="1.0" encoding="UTF-8"?>\n<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">'
        '<mime-type type="application/x-perplex-movie-studio"><comment>Perplex Movie Studio project</comment>'
        '<glob pattern="*.pmsproj"/></mime-type></mime-info>\n')
    deb = out.parent / f"PerplexMovieStudio-{version}-Linux-amd64.deb"
    subprocess.run(["dpkg-deb", "--root-owner-group", "--build", str(debroot), str(deb)], check=True)
    with tarfile.open(str(out) + ".tar.gz", "w:gz") as tar:
        tar.add(out, arcname=out.name)
    print("Verified Linux packages: " + str(deb))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=("assemble", "archive"))
    for name in ("build", "qt", "ffmpeg", "output"):
        parser.add_argument("--" + name, required=True, type=lambda p: Path(p).resolve())
    parser.add_argument("--verification", type=lambda p: Path(p).resolve())
    args = parser.parse_args()
    (assemble if args.phase == "assemble" else archive)(args)
