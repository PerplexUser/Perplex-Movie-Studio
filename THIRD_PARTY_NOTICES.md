# Third-party notices

Only original Perplex Movie Studio code is covered by the root MIT LICENSE.
Dependency licenses and copyrights remain with their respective authors.

| Component | Version | License / source |
|---|---|---|
| Qt Core, Gui, Concurrent, Widgets, Network, OpenGL, Multimedia, MultimediaWidgets; Windows platform/plugin support | 6.8.3 | LGPL-3.0; The Qt Company Ltd. and contributors. https://download.qt.io/archive/qt/6.8/6.8.3/submodules/ |
| Qt German base translation | 6.8.3 | Qt translation terms included with qttranslations source and notices |
| FFmpeg programs and libav* / libsw* libraries | 7.1.5 | LGPL-2.1-or-later. https://ffmpeg.org/releases/ffmpeg-7.1.5.tar.xz |
| LAME libmp3lame | 3.100 | LGPL-2.0-or-later. https://lame.sourceforge.io/ and https://deb.debian.org/debian/pool/main/l/lame/lame_3.100.orig.tar.gz |
| GCC libgcc / libstdc++ runtime | 14.2.0 | GPL-3.0 with GCC Runtime Library Exception 3.1. https://gcc.gnu.org/releases.html |
| MinGW-w64 runtime and winpthreads | 12.0.0 | Component-specific permissive/public-domain licenses, including ZPL-2.1/MIT/BSD terms; see supplied copyright notices. https://www.mingw-w64.org/ |
| NSIS installer stub | 3.11 | zlib/libpng license and bundled-component notices. https://nsis.sourceforge.io/ |

The exact source archives for Qtbase, Qt Multimedia, Qt translations, FFmpeg and
LAME are included in the accompanying source ZIP in `third-party/sources`, with
SHA-256 hashes in `third-party/source-manifest.json`. Qt's license, copyright and
attribution files are preserved in `third-party/licenses`. In the binary package
they are in `licenses`. These include upstream bundled-component notices, which
must remain with redistributions. LGPL/GPL texts required by LGPL are included.

FFmpeg is built without `--enable-gpl` or `--enable-nonfree`, and with network
support disabled. It does **not** contain libx264 or libx265. It uses the native
AAC/MPEG-4 encoders and LAME for MP3. Revision 2 does not select Media Foundation
for output. It disables optimized CPU paths at runtime to avoid pixel corruption.
The application communicates with the programs through a local process interface.
Qt playback uses the same supplied ABI-compatible FFmpeg DLLs. All these DLLs
remain separate and replaceable; reverse engineering for debugging modifications
to LGPL components is not restricted by this project.

See `packaging/build_windows_media.sh`, `docs/BUILD.md`, the preserved FFmpeg
configuration and `third-party/patches/lame-export-symbol.patch`. Original FFmpeg
source is unmodified. LAME has the one documented export-list correction.
Qt SDK binaries are unmodified; Qt6 does not receive application source patches.
SDK download provenance and component versions are recorded in `docs/SBOM.json`.

Application icons are original geometric artwork. Synthetic fixture/example media
are CC0-1.0.

MIT and open-source component licenses do not by themselves grant all codec
patent rights. The supplied development build is not an assertion that the
specification's distribution, patent and stable-release approval gates are complete.
Do not remove this notice, the supplied license directory or corresponding-source
information when redistributing. See FFmpeg's official guidance:
https://ffmpeg.org/legal.html and Qt's licensing information: https://www.qt.io/licensing/.
