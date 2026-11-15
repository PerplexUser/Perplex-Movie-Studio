#!/usr/bin/env python3
"""Build and test a native Windows x64 package using Qt's matching MinGW SDK."""
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

if os.name != "nt":
    raise SystemExit("Run on Windows x64 with MSYS2, Qt 6.8.3 MinGW and NSIS.")
root = Path(__file__).resolve().parents[1]
os.chdir(root)
version = (root / "VERSION").read_text().strip()
qt = Path(os.environ["QT_ROOT_DIR"]).resolve()
mingw = Path(os.environ["PMS_MINGW_ROOT"]).resolve()
build = root / "build/windows-x64"
media = root / "build/media-install-windows"
package = root / f"dist/PerplexMovieStudio-{version}-Windows-x64"
evidence = root / "build/verification-windows"
if build.exists() or package.exists() or evidence.exists():
    raise SystemExit("Use a fresh build checkout.")
evidence.mkdir(parents=True)
env = os.environ.copy()
env["PATH"] = str(mingw / "bin") + os.pathsep + str(qt / "bin") + os.pathsep + env["PATH"]

def run(command, log, timeout=3600):
    print("BUILD " + log, flush=True)
    with (evidence / (log + ".log")).open("w", encoding="utf-8") as stream:
        result = subprocess.run([str(x) for x in command], env=env, stdout=stream,
                                stderr=subprocess.STDOUT, timeout=timeout)
    if result.returncode:
        print((evidence / (log + ".log")).read_text(errors="replace")[-24000:])
        raise SystemExit(result.returncode)

assert subprocess.check_output([str(qt / "bin/qmake.exe"), "-query", "QT_VERSION"], env=env).strip() == b"6.8.3"
gcc = subprocess.check_output([str(mingw / "bin/gcc.exe"), "-dumpfullversion"], env=env).decode().strip()
if gcc != "13.1.0":
    raise SystemExit("The reviewed Qt MinGW 13.1.0 toolchain is required: " + gcc)
run([sys.executable, "packaging/prepare_sources.py"], "sources")
env.update(PMS_QT_MINGW=str(mingw), PMS_MEDIA_BUILD=str(root / "build/media-windows"),
           PMS_MEDIA_INSTALL=str(media), MSYSTEM="MINGW64", PMS_BUILD_JOBS="3")
script = '''export PATH="$(cygpath -u "$PMS_QT_MINGW")/bin:/usr/bin:/bin"
export PMS_MEDIA_BUILD="$(cygpath -u "$PMS_MEDIA_BUILD")"
export PMS_MEDIA_INSTALL="$(cygpath -u "$PMS_MEDIA_INSTALL")"
bash packaging/build_windows_media.sh
'''
if not (media / "config.h").is_file():
    run([r"C:\msys64\usr\bin\bash.exe", "--noprofile", "--norc", "-c", script], "media")
else:
    config = (media / "config.h").read_text()
    for flag in ("GPL", "NONFREE", "NETWORK", "VERSION3"):
        if f"#define CONFIG_{flag} 0" not in config.splitlines():
            raise SystemExit("Cached media configuration does not match the reviewed build")
    print("Using cached reviewed media build", flush=True)
run(["cmake", "-S", ".", "-B", build, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
     "-DBUILD_TESTING=ON", "-DCMAKE_PREFIX_PATH=" + str(qt),
     "-DCMAKE_CXX_COMPILER=" + str(mingw / "bin/g++.exe")], "configure")
run(["cmake", "--build", build, "--parallel", "3"], "compile")
run([sys.executable, "scripts/check_translations.py"], "translations")
run([sys.executable, "packaging/package_windows.py", "--build", build, "--qt", qt,
     "--ffmpeg", media / "bin", "--mingw", mingw, "--output", package], "assemble")
run([sys.executable, "packaging/verify_windows.py", "--package", package,
     "--build", build, "--qt", qt, "--output", evidence / "runtime"], "runtime")
report = json.loads((evidence / "runtime/verification.json").read_text())
report["compiler"] = "MinGW GCC " + gcc
(package / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
(package / "SHA256SUMS.txt").write_text("\n".join(
    f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(package).as_posix()}"
    for p in sorted(package.rglob("*")) if p.is_file() and p.name != "SHA256SUMS.txt") + "\n")
installer = package.with_name(package.name + "-Setup.exe")
nsis = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "NSIS/makensis.exe"
run([sys.executable, "packaging/make_installer.py", package, installer, "--makensis", nsis], "installer")
# Verify the real installer and its explicit uninstall list, preserving user-created files.
installed = root / "build/installed-smoke"
run([installer, "/S", "/D=" + str(installed)], "install")
if hashlib.sha256((installed / "PerplexMovieStudio.exe").read_bytes()).digest() != hashlib.sha256((package / "PerplexMovieStudio.exe").read_bytes()).digest():
    raise SystemExit("Installed executable does not match the tested package")
env["QT_QPA_PLATFORM"] = "windows"
env["PATH"] = str(installed) + os.pathsep + os.environ["SystemRoot"] + r"\System32"
run([installed / "PerplexMovieStudio.exe", "--self-test-ui"], "installed-ui", 60)
(installed / "user-project.txt").write_text("Keep this user file.")
run([installed / "Uninstall.exe", "/S", "_?=" + str(installed)], "uninstall", 120)
if (installed / "PerplexMovieStudio.exe").exists() or not (installed / "user-project.txt").exists():
    raise SystemExit("Uninstall did not preserve the user file or left the app executable")
report["installer"] = {"install": True, "launch": True, "uninstall_preserves_user_files": True}
report["nsis"] = subprocess.check_output([str(nsis), "/VERSION"]).decode().strip()
(evidence / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
shutil.make_archive(str(package), "zip", package.parent, package.name)
env["PATH"] = os.environ["PATH"]
run([sys.executable, "packaging/source_archive.py"], "source-archive")
print("Verified Windows installer and portable ZIP: " + str(installer))
