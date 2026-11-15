#!/usr/bin/env bash
set -euo pipefail
[[ "$(uname -s)" == Linux && "$(uname -m)" == x86_64 ]] || { echo 'Native Linux x86_64 required.' >&2; exit 1; }
project_root="$(cd "$(dirname "$0")/.." && pwd)"
build_root="${PMS_MEDIA_BUILD:?Set an empty media build directory}"
install_root="${PMS_MEDIA_INSTALL:?Set an empty installation directory}"
[[ ! -e "$build_root/lame-build" ]] || { echo 'Use a fresh media build directory.' >&2; exit 1; }
mkdir -p "$build_root" "$install_root"
tar -xf "$project_root/third-party/sources/lame-3.100.tar.gz" -C "$build_root"
tar -xf "$project_root/third-party/sources/ffmpeg-7.1.5.tar.xz" -C "$build_root"
patch -d "$build_root/lame-3.100" -p1 < "$project_root/third-party/patches/lame-export-symbol.patch"
mkdir "$build_root/lame-build" "$build_root/ffmpeg-build"
cd "$build_root/lame-build"
../lame-3.100/configure --prefix="$install_root" --enable-shared --disable-static --disable-frontend --disable-nasm CFLAGS='-O2 -fPIC'
make -j"${PMS_BUILD_JOBS:-3}"
make install
cd "$build_root/ffmpeg-build"
../ffmpeg-7.1.5/configure --prefix="$install_root" --enable-shared --disable-static --enable-pic \
  --disable-gpl --disable-nonfree --disable-autodetect --disable-network --disable-doc \
  --disable-debug --disable-x86asm --enable-libmp3lame \
  --extra-cflags="-I$install_root/include" --extra-ldflags="-L$install_root/lib"
make -j"${PMS_BUILD_JOBS:-3}"
make install
for flag in GPL NONFREE NETWORK VERSION3; do
  grep -q "^#define CONFIG_${flag} 0$" config.h
done
cp config.h config_components.h "$install_root/"
cp ffbuild/config.log "$install_root/build-config.log"
