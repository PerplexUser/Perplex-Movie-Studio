#!/usr/bin/env python3
"""Verify the supplied upstream sources; join the repository's split Qt ZIP if needed."""
import hashlib
import io
import json
from pathlib import Path
import tarfile
import urllib.request
import zipfile

root = Path(__file__).resolve().parents[1]
sources = root / "third-party/sources"
qt = sources / "qtbase-everywhere-src-6.8.3.tar.xz"
if not qt.exists():
    parts = sorted(sources.glob("qtbase-everywhere-src-6.8.3.tar.zip.*"))
    if len(parts) != 5:
        raise SystemExit("Missing complete Qt source archive or its five repository parts")
    with zipfile.ZipFile(io.BytesIO(b"".join(p.read_bytes() for p in parts))) as archive:
        # Read just the expected member, never extract arbitrary archive paths.
        data = archive.read(qt.name)
    expected = "56001b905601bb9023d399f3ba780d7fa940f3e4861e496a7c490331f49e0b80"
    if hashlib.sha256(data).hexdigest() != expected:
        raise SystemExit("Reassembled Qt source checksum mismatch")
    qt.write_bytes(data)
for entry in json.loads((root / "third-party/source-manifest.json").read_text()):
    path = sources / entry["file"]
    if not path.exists() and entry.get("url"):
        # The complete delivery ZIP includes this file. Git checkouts fetch the
        # large upstream archive once, with an exact pinned size and SHA-256.
        temporary = path.with_suffix(path.suffix + ".download")
        try:
            with urllib.request.urlopen(entry["url"], timeout=120) as response, temporary.open("wb") as target:
                total = 0
                while chunk := response.read(1024 * 1024):
                    total += len(chunk)
                    if total > entry["size"]:
                        raise RuntimeError("Source download exceeds its pinned size")
                    target.write(chunk)
            if temporary.stat().st_size != entry["size"] or hashlib.sha256(temporary.read_bytes()).hexdigest() != entry["sha256"]:
                raise RuntimeError("Downloaded source checksum mismatch: " + path.name)
            temporary.replace(path)
        finally:
            temporary.unlink(missing_ok=True)
    if path.stat().st_size != entry["size"] or hashlib.sha256(path.read_bytes()).hexdigest() != entry["sha256"]:
        raise SystemExit(f"Source checksum mismatch: {path.name}")
    print(f"Verified {path.name}")

# Qt Multimedia's backend also links QML/Quick. Preserve their corresponding
# source and upstream license/attribution files in every binary distribution.
with tarfile.open(sources / "qtdeclarative-everywhere-src-6.8.3.tar.xz") as archive:
    for member in archive.getmembers():
        relative = Path(*Path(member.name).parts[1:])
        if not member.isfile() or ".." in relative.parts:
            continue
        if (relative.parts and relative.parts[0] == "LICENSES") or relative.name in {"LICENSE", "COPYING", "NOTICE", "LICENSE.txt", "LICENSE.md", "qt_attribution.json"}:
            target = root / "third-party/licenses/qtdeclarative" / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(archive.extractfile(member).read())
