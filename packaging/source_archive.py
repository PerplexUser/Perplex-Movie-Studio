#!/usr/bin/env python3
"""Archive corresponding sources, including the complete verified Qt archive."""
from pathlib import Path
import zipfile
root = Path(__file__).resolve().parents[1]
version = (root / "VERSION").read_text().strip()
target = root / f"dist/PerplexMovieStudio-{version}-Source.zip"
target.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
    for path in sorted(root.rglob("*")):
        relative = path.relative_to(root)
        if relative.parts[0] in {".git", "build", "dist", ".deps", ".tools", "Qt"} or "__pycache__" in relative.parts:
            continue
        if ".tar.zip." in path.name or not path.is_file():
            continue
        archive.write(path, Path("PerplexMovieStudio") / relative)
print(target)
