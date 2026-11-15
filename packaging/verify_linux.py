#!/usr/bin/env python3
"""Verify the distributed Linux runtime independently of the development SDK."""
import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
from package_linux import hashes, patch

parser = argparse.ArgumentParser(description=__doc__)
for name in ("package", "build", "qt", "output"):
    parser.add_argument("--" + name, required=True, type=lambda p: Path(p).resolve())
a = parser.parse_args()
if platform.system() != "Linux" or platform.machine() != "x86_64":
    raise SystemExit("Native Linux x86_64 required")
a.output.mkdir(parents=True, exist_ok=False)
env = {k: v for k, v in os.environ.items() if not k.startswith(("QT_", "LD_", "PMS_"))}
env.update(QT_QPA_PLATFORM="offscreen", QT_MEDIA_BACKEND="ffmpeg", PATH="/usr/bin:/bin",
           QT_FFMPEG_DECODING_HW_DEVICE_TYPES=",", QT_DISABLE_HW_TEXTURES_CONVERSION="1")
report = {"version": (Path(__file__).resolve().parents[1] / "VERSION").read_text().strip(),
          "platform": platform.platform(), "distribution": platform.freedesktop_os_release(),
          "architecture": "x86_64", "passed": False, "release_sha256": hashes(a.package),
          "scope": "Native automated runtime tests; physical audio, manual desktop and long-term performance remain separate.", "cases": []}

def case(name, command, timeout=300):
    print("VERIFY " + name, flush=True)
    with (a.output / (name + ".log")).open("w") as stream:
        try:
            result = subprocess.run([str(x) for x in command], env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=timeout)
            detail = {"name": name, "passed": result.returncode == 0, "exit_code": result.returncode}
        except subprocess.TimeoutExpired:
            detail = {"name": name, "passed": False, "error": "timeout"}
    report["cases"].append(detail)
    if not detail["passed"]:
        print((a.output / (name + ".log")).read_text(errors="replace"))

case("packaged_ui", [a.package / "PerplexMovieStudio", "--self-test-ui"])
case("packaged_probe", [a.package / "PerplexMovieStudio", "--probe", a.build / "fixtures/h264-aac.mp4"])
case("packaged_ffmpeg", [a.package / "bin/ffmpeg", "-version"])
env["QT_QPA_PLATFORM"] = "xcb"
case("packaged_x11", ["/usr/bin/xvfb-run", "-a", a.package / "PerplexMovieStudio", "--self-test-ui"])
env["QT_QPA_PLATFORM"] = "offscreen"
isolated = a.output / "Moved folder with spaces"
shutil.copytree(a.package, isolated)
names = ("domain_tests", "service_tests", "preview_tests", "feature_tests", "window_tests", "file_open_tests")
for name in names:
    shutil.copy2(a.build / name, isolated / "bin" / name)
    patch(isolated / "bin" / name, "$ORIGIN/../lib")
shutil.copy2(a.qt / "lib/libQt6Test.so.6", isolated / "lib/libQt6Test.so.6")
patch(isolated / "lib/libQt6Test.so.6", "$ORIGIN")
shutil.copytree(a.build / "fixtures", isolated / "bin/fixtures")
shutil.copy2(a.build / "perplex_de_DE.qm", isolated / "bin")
env.update(LD_LIBRARY_PATH=str(isolated / "lib"), QT_PLUGIN_PATH=str(isolated / "plugins"),
           PMS_FFMPEG=str(isolated / "bin/ffmpeg"), PMS_FFPROBE=str(isolated / "bin/ffprobe"))
for name in names:
    case(name, [isolated / "bin" / name])
report["passed"] = all(c["passed"] for c in report["cases"])
(a.output / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
if not report["passed"]:
    raise SystemExit(1)
