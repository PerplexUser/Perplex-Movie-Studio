#!/usr/bin/env bash
set -euo pipefail
[[ "$(uname -s)" == Darwin ]] || { echo 'Build on macOS with Xcode Command Line Tools.' >&2; exit 1; }
project_root="$(cd "$(dirname "$0")/.." && pwd)"
build_root="${PMS_MEDIA_BUILD:?Set PMS_MEDIA_BUILD to an empty directory}"
install_root="${PMS_MEDIA_INSTALL:?Set PMS_MEDIA_INSTALL to an empty output directory}"
arch="$(uname -m)"
case "$arch" in
  arm64) host=aarch64-apple-darwin; ffarch=aarch64 ;;
  x86_64) host=x86_64-apple-darwin; ffarch=x86_64 ;;
  *) echo "Unsupported architecture: $arch" >&2; exit 1 ;;
esac
[[ ! -e "$build_root/lame-build" ]] || { echo 'Use a fresh media build directory.' >&2; exit 1; }
mkdir -p "$build_root" "$install_root"
sdk="$(xcrun --sdk macosx --show-sdk-path)"
export SDKROOT="$sdk"
export MACOSX_DEPLOYMENT_TARGET=12.0
export CC="$(xcrun --find clang)"
export CPPFLAGS="-arch $arch -isysroot $sdk -mmacosx-version-min=12.0"
export CFLAGS="-O2 -arch $arch -isysroot $sdk -mmacosx-version-min=12.0"
export LDFLAGS="-arch $arch -isysroot $sdk -mmacosx-version-min=12.0 -Wl,-headerpad_max_install_names"
tar -xf "$project_root/third-party/sources/lame-3.100.tar.gz" -C "$build_root"
tar -xf "$project_root/third-party/sources/ffmpeg-7.1.5.tar.xz" -C "$build_root"
patch -d "$build_root/lame-3.100" -p1 < "$project_root/third-party/patches/lame-export-symbol.patch"
mkdir "$build_root/lame-build" "$build_root/ffmpeg-build"
cd "$build_root/lame-build"
../lame-3.100/configure --build="$host" --host="$host" --prefix="$install_root" \
  --enable-shared --disable-static --disable-frontend --disable-nasm
# Autoconf runs the preprocessor separately, without CFLAGS. Its header tests
# must use the SDK too; otherwise it selects obsolete bcopy fallbacks.
grep -q '^#define STDC_HEADERS 1$' config.h
make -j"${PMS_BUILD_JOBS:-3}"
make install
cd "$build_root/ffmpeg-build"
../ffmpeg-7.1.5/configure --prefix="$install_root" --target-os=darwin --arch="$ffarch" \
  --cc="$CC" --host-cc="$CC" --host-cflags="$CFLAGS" --host-ldflags="$LDFLAGS" \
  --enable-shared --disable-static --disable-gpl --disable-nonfree \
  --disable-autodetect --disable-network --disable-doc --disable-debug --disable-x86asm \
  --enable-videotoolbox --enable-libmp3lame --extra-cflags="$CFLAGS -I$install_root/include" \
  --extra-ldflags="$LDFLAGS -L$install_root/lib"
# Qt's macOS plugin imports VideoToolbox pixel-format mapping symbols even
# when playback is configured to use software decoding.
grep -q '^#define CONFIG_VIDEOTOOLBOX 1$' config.h
make -j"${PMS_BUILD_JOBS:-3}"
make install
nm -gU "$install_root/lib/libavutil.59.dylib" > "$install_root/avutil-symbols.txt"
grep -q ' _av_map_videotoolbox_format_to_pixfmt$' "$install_root/avutil-symbols.txt"
for flag in GPL NONFREE NETWORK VERSION3; do
  grep -q "^#define CONFIG_${flag} 0$" config.h
done
cp config.h config_components.h "$install_root/"
cp ffbuild/config.log "$install_root/build-config.log"
