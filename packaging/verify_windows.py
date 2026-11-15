#!/usr/bin/env python3
"""Execute the exact Windows package and tests from a relocated runtime."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
for key in ("package", "build", "qt", "output"):
    parser.add_argument("--" + key, required=True, type=lambda p: Path(p).resolve())
a = parser.parse_args()
if os.name != "nt":
    raise SystemExit("Native Windows verification is required.")
a.output.mkdir(parents=True, exist_ok=False)
env = {k: v for k, v in os.environ.items() if not k.startswith(("QT_", "PMS_"))}
env.update(QT_QPA_PLATFORM="windows", QT_MEDIA_BACKEND="ffmpeg",
           QT_FFMPEG_DECODING_HW_DEVICE_TYPES=",", QT_DISABLE_HW_TEXTURES_CONVERSION="1")
env["PATH"] = str(a.package) + os.pathsep + os.environ["SystemRoot"] + r"\System32"
report = {"version": (Path(__file__).resolve().parents[1] / "VERSION").read_text().strip(),
          "platform": platform.platform(), "architecture": platform.machine(), "passed": False,
          "scope": "Native automated editor, playback and render tests; physical audio, manual desktop and performance acceptance are separate.",
          "release_sha256": {p.relative_to(a.package).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                             for p in a.package.rglob("*") if p.suffix.lower() in (".dll", ".exe")}, "cases": []}

def case(name, command, timeout=300):
    print("VERIFY " + name, flush=True)
    with (a.output / (name + ".log")).open("w", encoding="utf-8") as stream:
        try:
            result = subprocess.run([str(x) for x in command], env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=timeout)
            detail = {"name": name, "passed": result.returncode == 0, "exit_code": result.returncode}
        except subprocess.TimeoutExpired:
            detail = {"name": name, "passed": False, "error": "timeout"}
    report["cases"].append(detail)
    if not detail["passed"]:
        print((a.output / (name + ".log")).read_text(errors="replace"))

case("packaged_ui", [a.package / "PerplexMovieStudio.exe", "--self-test-ui"])
case("packaged_probe", [a.package / "PerplexMovieStudio.exe", "--probe", a.build / "fixtures/h264-aac.mp4"])
case("packaged_ffmpeg", [a.package / "ffmpeg.exe", "-version"])
isolated = a.output / "Moved folder with spaces"
shutil.copytree(a.package, isolated)
names = ("domain_tests", "service_tests", "preview_tests", "feature_tests", "window_tests", "file_open_tests")
for name in names:
    shutil.copy2(a.build / (name + ".exe"), isolated)
shutil.copy2(a.qt / "bin/Qt6Test.dll", isolated)
shutil.copytree(a.build / "fixtures", isolated / "fixtures")
shutil.copy2(a.build / "perplex_de_DE.qm", isolated)
env.update(PATH=str(isolated) + os.pathsep + os.environ["SystemRoot"] + r"\System32",
           PMS_FFMPEG=str(isolated / "ffmpeg.exe"), PMS_FFPROBE=str(isolated / "ffprobe.exe"))
for name in names:
    command = [isolated / (name + ".exe")]
    # Windows GUI-subsystem QtTest binaries do not reliably inherit stdout.
    qt_log = a.output / (name + ".qtest.log")
    if name != "domain_tests":
        command += ["-o", str(qt_log) + ",txt"]
    case(name, command)
    if not report["cases"][-1]["passed"] and qt_log.exists():
        print(qt_log.read_text(errors="replace"))
report["passed"] = all(c["passed"] for c in report["cases"])
(a.output / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
if not report["passed"]:
    raise SystemExit(1)
