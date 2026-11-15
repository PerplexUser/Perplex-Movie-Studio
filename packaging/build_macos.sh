#!/usr/bin/env bash
set -euo pipefail
[[ "$(uname -s)" == Darwin ]] || { echo 'A native Mac with Xcode Command Line Tools is required.' >&2; exit 1; }
project_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$project_root"
arch="$(uname -m)"
case "$arch" in arm64|x86_64) ;; *) echo "Unsupported architecture $arch" >&2; exit 1 ;; esac
qt_root="${QT_ROOT_DIR:?Set QT_ROOT_DIR to the Qt 6.8.3 macOS SDK}"
[[ "$("$qt_root/bin/qmake" -query QT_VERSION)" == 6.8.3 ]] || { echo 'Qt 6.8.3 required.' >&2; exit 1; }
[[ "$project_root" != *' '* && "$qt_root" != *' '* ]] || { echo 'Build paths must not contain spaces (installed app paths may).' >&2; exit 1; }
build="$project_root/build/macos-$arch"
media_build="$project_root/build/media-$arch"
media_install="$project_root/build/media-install-$arch"
version="$(cat "$project_root/VERSION")"
package="$project_root/dist/PerplexMovieStudio-$version-macOS-$arch"
verification="$project_root/build/verification-$arch"
[[ ! -e "$build" && ! -e "$package" && ! -e "$media_build" && ! -e "$verification" ]] || { echo 'Use a fresh build checkout.' >&2; exit 1; }
python3 packaging/prepare_sources.py
python3 packaging/macos/make_icon.py
PMS_MEDIA_BUILD="$media_build" PMS_MEDIA_INSTALL="$media_install" bash packaging/build_macos_media.sh
cmake -S . -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DCMAKE_PREFIX_PATH="$qt_root" -DCMAKE_OSX_ARCHITECTURES="$arch" -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  -DCMAKE_OSX_SYSROOT="$(xcrun --sdk macosx --show-sdk-path)"
cmake --build "$build" --parallel "${PMS_BUILD_JOBS:-3}"
python3 scripts/check_translations.py
python3 packaging/package_macos.py assemble --arch "$arch" --build "$build" --qt "$qt_root" \
  --ffmpeg "$media_install" --output "$package" --sign-identity "${PMS_CODESIGN_IDENTITY:--}"
python3 packaging/verify_macos.py --arch "$arch" --bundle "$package/Perplex Movie Studio.app" \
  --build "$build" --qt "$qt_root" --output "$verification"
python3 packaging/package_macos.py archive --arch "$arch" --output "$package" \
  --verification "$verification/verification.json"
python3 packaging/source_archive.py
echo "Verified archives: $package.zip and $package.dmg"
