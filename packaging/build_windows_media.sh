#!/usr/bin/env bash
set -euo pipefail
# Run on Linux with MinGW-w64 POSIX compilers in PATH, GNU make and tar.
# Inputs are the matching unmodified upstream source archives included in the source ZIP.
project_root="$(cd "$(dirname "$0")/.." && pwd)"
build_root="${PMS_MEDIA_BUILD:?Set PMS_MEDIA_BUILD to an empty build directory}"
install_root="${PMS_MEDIA_INSTALL:?Set PMS_MEDIA_INSTALL to the output directory}"
mkdir -p "$build_root" "$install_root"
tar -xf "$project_root/third-party/sources/lame-3.100.tar.gz" -C "$build_root"
tar -xf "$project_root/third-party/sources/ffmpeg-7.1.5.tar.xz" -C "$build_root"
patch -d "$build_root/lame-3.100" -p1 < "$project_root/third-party/patches/lame-export-symbol.patch"
mkdir -p "$build_root/lame-build" "$build_root/ffmpeg-build"
cd "$build_root/lame-build"
../lame-3.100/configure --host=x86_64-w64-mingw32 --prefix="$install_root" --enable-shared --disable-static --disable-frontend
make -j"${PMS_BUILD_JOBS:-4}"
make install
cd "$build_root/ffmpeg-build"
../ffmpeg-7.1.5/configure --prefix="$install_root" --target-os=mingw32 --arch=x86_64 \
  --enable-cross-compile --cross-prefix=x86_64-w64-mingw32- --enable-shared --disable-static \
  --disable-gpl --disable-nonfree --disable-autodetect --disable-network --disable-doc \
  --disable-debug --disable-x86asm --enable-libmp3lame --enable-mediafoundation \
  --extra-cflags="-I$install_root/include" --extra-ldflags="-L$install_root/lib"
make -j"${PMS_BUILD_JOBS:-4}"
make install
# A hard build-configuration gate, independent of executable signing or release approval.
grep -q '^#define CONFIG_GPL 0$' config.h
grep -q '^#define CONFIG_NONFREE 0$' config.h
grep -q '^#define CONFIG_NETWORK 0$' config.h
cp config.h config_components.h "$install_root/"
cp ffbuild/config.log "$install_root/build-config.log"
