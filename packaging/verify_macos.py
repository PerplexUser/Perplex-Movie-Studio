#!/usr/bin/env python3
"""Exercise the packaged runtime, then integration tests against a relocated copy."""
import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
from package_macos import macho_hashes, relocate_and_sign

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("bundle", "build", "qt", "output"):
        parser.add_argument("--" + name, type=lambda p: Path(p).resolve(), required=True)
    parser.add_argument("--arch", required=True, choices=("arm64", "x86_64"))
    args = parser.parse_args()
    if platform.system() != "Darwin" or platform.machine() != args.arch:
        raise SystemExit("Verification requires a native Mac matching --arch")
    args.output.mkdir(parents=True, exist_ok=False)
    report = {"platform": platform.system(), "os_version": platform.mac_ver()[0],
              "architecture": args.arch, "passed": False, "cases": [],
              "release_macho_sha256": macho_hashes(args.bundle),
              "scope": "Native automated offscreen playback/render/editor tests; manual desktop, physical audio and Gatekeeper acceptance remain open."}
    env = {k: v for k, v in os.environ.items() if not k.startswith(("QT_", "DYLD_", "PMS_"))}
    env.update(QT_QPA_PLATFORM="offscreen", QT_MEDIA_BACKEND="ffmpeg", QT_DEBUG_PLUGINS="1",
               QT_FFMPEG_DECODING_HW_DEVICE_TYPES=",", QT_DISABLE_HW_TEXTURES_CONVERSION="1")
    def case(name, command, timeout=300):
        print(f"VERIFY {name}", flush=True)
        with (args.output / (name + ".log")).open("w") as log:
            try:
                result = subprocess.run([str(p) for p in command], env=env, stdout=log,
                                        stderr=subprocess.STDOUT, timeout=timeout, check=False)
                passed = result.returncode == 0
                detail = {"name": name, "passed": passed, "exit_code": result.returncode}
            except subprocess.TimeoutExpired:
                detail = {"name": name, "passed": False, "error": "timeout"}
        report["cases"].append(detail)
        if not detail["passed"]:
            print((args.output / (name + ".log")).read_text(errors="replace"), flush=True)
        return detail["passed"]
    # These checks execute the exact sealed release app before adding any test libraries.
    case("packaged_ui", [args.bundle / "Contents/MacOS/PerplexMovieStudio", "--self-test-ui"])
    case("packaged_probe", [args.bundle / "Contents/MacOS/PerplexMovieStudio", "--probe",
                            args.build / "fixtures/h264-aac.mp4"])
    case("packaged_ffmpeg", [args.bundle / "Contents/MacOS/media/ffmpeg", "-version"])
    isolated = args.output / "Moved folder with spaces" / "Perplex Movie Studio.app"
    shutil.copytree(args.bundle, isolated, symlinks=True)
    binaries = isolated / "Contents/MacOS"
    names = ("domain_tests", "service_tests", "preview_tests", "feature_tests", "window_tests", "file_open_tests")
    for name in names:
        shutil.copy2(args.build / name, binaries / name)
    resources = isolated / "Contents/Resources"
    shutil.copytree(args.build / "fixtures", resources / "test-fixtures")
    shutil.copy2(args.build / "perplex_de_DE.qm", resources / "translations/perplex_de_DE.qm")
    shutil.copytree(args.qt / "lib/QtTest.framework", isolated / "Contents/Frameworks/QtTest.framework", symlinks=True)
    relocate_and_sign(isolated, args.arch)
    env.update(QT_PLUGIN_PATH=str(isolated / "Contents/PlugIns"),
               PMS_TEST_FIXTURES=str(resources / "test-fixtures"),
               PMS_FFMPEG=str(binaries / "media/ffmpeg"), PMS_FFPROBE=str(binaries / "media/ffprobe"))
    for name in names:
        case(name, [binaries / name])
    report["passed"] = all(c["passed"] for c in report["cases"])
    (args.output / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"passed": report["passed"], "cases": report["cases"]}, indent=2))
    if not report["passed"]:
        raise SystemExit(1)

if __name__ == "__main__":
    main()
