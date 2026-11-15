#!/usr/bin/env bash
set -euo pipefail
[[ "$(uname -s)" == Linux && "$(uname -m)" == x86_64 ]] || { echo 'Native Linux x86_64 required.' >&2; exit 1; }
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
qt="${QT_ROOT_DIR:?Set the Qt 6.8.3 gcc_64 SDK root}"
[[ "$("$qt/bin/qmake" -query QT_VERSION)" == 6.8.3 ]] || exit 1
version="$(cat VERSION)"
build="$root/build/linux-x64"
media="$root/build/media-install-linux"
package="$root/dist/PerplexMovieStudio-$version-Linux-x64"
evidence="$root/build/verification-linux"
[[ ! -e "$build" && ! -e "$package" ]] || { echo 'Use a fresh checkout.' >&2; exit 1; }
python3 packaging/prepare_sources.py
PMS_MEDIA_BUILD="$root/build/media-linux" PMS_MEDIA_INSTALL="$media" bash packaging/build_linux_media.sh
cmake -S . -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH="$qt"
cmake --build "$build" --parallel "${PMS_BUILD_JOBS:-3}"
python3 scripts/check_translations.py
args=(--build "$build" --qt "$qt" --ffmpeg "$media" --output "$package")
python3 packaging/package_linux.py assemble "${args[@]}"
python3 packaging/verify_linux.py --package "$package" --build "$build" --qt "$qt" --output "$evidence"
python3 packaging/package_linux.py archive "${args[@]}" --verification "$evidence/verification.json"
python3 packaging/source_archive.py
