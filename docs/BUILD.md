# Reproducible build guide

## Versions used for this delivery

| Component | Version / configuration |
|---|---|
| Application | 0.1.1-dev Revision 2, C++20 |
| CMake | 3.31.6 |
| Native test compiler | Debian GCC 14.2.0, Linux x86_64 |
| Windows cross compiler | MinGW-w64 GCC 14.2.0 POSIX, mingw-w64 12.0.0 |
| Qt SDK | 6.8.3, gcc_64 host and win64_mingw target, shared LGPL modules |
| FFmpeg | 7.1.5, LGPL 2.1+, shared Windows libraries, no network/GPL/nonfree |
| LAME | 3.100, shared, one documented export-symbol patch |
| Installer | NSIS 3.11 |
| Python packaging | Python 3.12, pefile 2024.8.26 |
| Formatting only | clang-format 19.1.7 |

The SDK version is pinned for reproducibility, not advertised as the newest Qt
release. Dependency security updates and target-platform retesting are release
gates. Build tools are not required on an end user's machine.

## Windows native build

Install MSVC 2022 x64, CMake and **Qt 6.8.3 for MSVC 2022 x64** with Qt Multimedia
and Linguist tools. Set `QT_ROOT_DIR` to that SDK root; do not use MinGW Qt with
MSVC. The source package includes upstream source archives if rebuilding Qt is
preferred. From a VS 2022 developer shell:

```powershell
$env:QT_ROOT_DIR = 'C:\Qt\6.8.3\msvc2022_64'
cmake --preset windows
cmake --build --preset windows
$env:PMS_FFMPEG = 'C:\media-tools\ffmpeg.exe'
$env:PMS_FFPROBE = 'C:\media-tools\ffprobe.exe'
ctest --preset windows
python scripts/check_translations.py
```

Alternatively use matching Qt 6.8.3 MinGW and MinGW GCC with a Ninja/MinGW
Makefiles generator. The MSVC preset is provided but was not executed in this
delivery. Do not describe it as tested.

## Linux native build and tests

Install Qt 6.8.3 shared SDK with qtbase, qttools, qttranslations and qtmultimedia,
CMake, a C++20 compiler and OpenGL development headers/libraries. With that SDK:

```sh
export QT_ROOT_DIR=/path/to/Qt/6.8.3/gcc_64
export PMS_FFMPEG=/path/to/lgpl/ffmpeg
export PMS_FFPROBE=/path/to/lgpl/ffprobe
cmake --preset linux
cmake --build --preset linux -j4
ctest --preset linux
python3 scripts/check_translations.py
```

The UI smoke test uses Qt's offscreen platform. A sound device is not needed for
the smoke test; it is needed for real playback acceptance. Use writable TMPDIR,
XDG_CONFIG_HOME, XDG_DATA_HOME and XDG_CACHE_HOME in a restricted container.
The pure-domain build does not require Qt: configure/build/test `domain-only`.

## Reproduce the delivered Windows cross build

The delivery was built on Linux, with the Windows MinGW Qt 6.8.3 SDK and the
matching Linux Qt 6.8.3 host tools. Example SDK acquisition during development:

```sh
python3 -m pip install aqtinstall==3.2.1 cmake==3.31.6 pefile==2024.8.26
python3 -m aqt install-qt linux desktop 6.8.3 linux_gcc_64 \
  --modules qtmultimedia --archives qtbase qttools qttranslations qtmultimedia icu -O Qt
python3 -m aqt install-qt windows desktop 6.8.3 win64_mingw \
  --modules qtmultimedia --archives qtbase qttools qttranslations qtmultimedia -O Qt
```

For the supplied toolchain file, `PMS_TOOLCHAIN_ROOT` contains an extracted
Debian-style `usr/bin/x86_64-w64-mingw32-g++-posix` and its sibling sysroot.
On a normal installed Linux cross-toolchain this root can be `/`.

```sh
export PMS_TOOLCHAIN_ROOT=/path/to/mingw-root
cmake -S . -B build/windows-cross \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/packaging/mingw-toolchain.cmake" \
  -DQT_HOST_PATH=/path/to/Qt/6.8.3/gcc_64 \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/mingw_64 \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/windows-cross -j4
```

Tests compiled for Windows must be **run on Windows**; cross-compilation is not
a substitute for execution. `service_tests.exe` needs the same Qt/FFmpeg DLLs
plus Qt6Test.dll and the copied `fixtures` folder. Native CTest handles its paths.

## Media tools, package and installer

`packaging/build_windows_media.sh` rebuilds FFmpeg and LAME from the exact source
archives in `third-party/sources`. It applies the included LAME patch, whose only
change removes an obsolete undefined `lame_init_old` export. The upstream archive
is unchanged; the patch is separate. Compiler binaries must be in PATH.

```sh
export PMS_MEDIA_BUILD=/absolute/empty/media-build
export PMS_MEDIA_INSTALL=/absolute/output/media-tools
bash packaging/build_windows_media.sh
python3 packaging/package_windows.py \
  --build build/windows-cross --qt /path/to/Qt/6.8.3/mingw_64 \
  --ffmpeg "$PMS_MEDIA_INSTALL/bin" --mingw "$PMS_TOOLCHAIN_ROOT/usr" \
  --output dist/PerplexMovieStudio-Windows-x64
python3 packaging/make_installer.py dist/PerplexMovieStudio-Windows-x64 \
  dist/PerplexMovieStudio-0.1.1-dev-r2-Windows-x64-Setup.exe
```

When NSIS is extracted rather than installed, set NSISDIR to its `share/nsis`
directory and pass `--makensis /path/to/makensis`. Package assembly checks the x64
machine type and recursively resolves every non-system normal/delayed PE import, including symbol names/ordinals against shipped exports.
Its report is `dependency-check.json`. This is a static check, not a Windows
loader execution test. The installer uses an explicit uninstall file list and
does not recursively erase arbitrary user files from the install directory.

## Localization, CLI and diagnostics

```sh
lupdate src -no-obsolete -ts translations/perplex_de_DE.ts
# Translate new messages in Qt Linguist, then:
python3 scripts/check_translations.py
cmake --build build/linux

PerplexMovieStudio --probe /path/to/local/media.mp4
PerplexMovieStudio --render /path/to/project.pmsproj --output /path/to/new.mp4
PerplexMovieStudio --self-test-ui
QT_QPA_PLATFORM=offscreen PerplexMovieStudio --screenshot /path/to/ui.png
```

CLI rendering never overwrites an existing destination. The graphical dialog
asks explicitly. `PMS_FFMPEG` and `PMS_FFPROBE` can point to compatible replacement
executables; otherwise the application checks its media subfolder, its own folder,
then PATH. No shell is used for media arguments.


## Revision 2 media policy

The dependency source/binary builds remain the versions above. The bundled FFmpeg was configured with `--disable-x86asm`; Revision 2 also forces `-cpuflags 0` in application processes. Qt playback resolves `av_force_cpu_flags` / `av_get_cpu_flags` from the loaded avutil-59 library and selects scalar paths before decoding. Keep that library ABI compatible when replacing dependencies. The Qt FFmpeg backend uses software decoding and software frame conversion; no hardware encoder is selected. MP4/MKV output is MPEG-4 Part 2/AAC.

QtConcurrent prepares title/image canvases off the UI thread. Qt image plugins provide JPEG/PNG decoding; the renderer passes raw RGBA frames to FFmpeg. Font rendering therefore requires QGuiApplication/QApplication even for CLI rendering. Offscreen native tests cover color conversion, all new effects, portrait output and an application workflow. Native test run: six CTest groups. Windows tests require the application's DLL/plugins plus Qt6Test.dll; copy test fixtures beside the executables.

If a restricted Linux build host prevents NSIS from using `/tmp`, the optional `packaging/nsis_workspace_temp.c` adapter routes its temporary files to TMPDIR:

```sh
cc -shared -fPIC packaging/nsis_workspace_temp.c -o /writable/path/nsis_workspace_temp.so
TMPDIR=/writable/path LD_PRELOAD=/writable/path/nsis_workspace_temp.so \
  python3 packaging/make_installer.py dist/PerplexMovieStudio-Windows-x64 \
  dist/PerplexMovieStudio-0.1.1-dev-r2-Windows-x64-Setup.exe
```

This helper is build-host-only and is not shipped in the Windows runtime. The installer uses a quoted uninstall command, a per-user destination and an explicit non-recursive uninstall file list.
