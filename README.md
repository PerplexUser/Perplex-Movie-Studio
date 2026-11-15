# Perplex Movie Studio

The 5 zip archives named qtbase-everywhere-src-6.8.3.tar.zip.001 through 005 must be extracted in the /third-party/sources/ directory.
Additionally, there are 4 zip archives in the same directory—named qtdeclarative-everywhere-src-6.8.3.tar.zip.001 through 004—which must also be extracted.

Installation files: https://github.com/PerplexUser/Perplex-Movie-Studio/releases/tag/v1.0.0

macOS: native ARM64/Intel build and packaging instructions are in [docs/MACOS_DE.md](docs/MACOS_DE.md). The macOS bundles are ad-hoc signed, not notarized. Consult the matching build verification report for actual results.

**1.0.0** — local video editor by Michael Lappenbusch · www.perplex.click.

## Start

Linux Mint/Ubuntu: [Installation and package build](docs/LINUX_DE.md).
macOS: [Apple Silicon and Intel installation](docs/MACOS_DE.md).

Run `PerplexMovieStudio-1.0.0-Windows-x64-Setup.exe`. Installation is for the current user and creates a Start menu shortcut. Alternatively extract the entire portable ZIP and run `PerplexMovieStudio.exe`. Keep the EXE, DLLs, plugins, translations and licenses together. No Python, SDK, account or internet connection is needed.

Windows 10 22H2 / Windows 11 x64 are the target systems. The About dialog, executable metadata and package filenames use **1.0.0**. See the matching verification report for the operating systems and tests actually exercised. Windows installers are unsigned; macOS bundles are ad-hoc signed and not notarized.

The initial language is English. Choose **Settings → Language → Deutsch** for German; the choice is remembered.

## Version 1.0.0

- Fixed title typing: delayed saves retain the cursor, selection and local undo history.
- Added per-clip **Picture → Opacity (%)**: 100% is opaque, 0% is invisible. Values in between reveal lower video tracks. Existing PNG transparency and transitions are multiplied by this value; audio volume is independent.
- Unified application, About dialog and installer version at **1.0.0**, without a development label.

## Existing features

- Corrected color handling for preview and export. The media pipeline explicitly selects software decoding, scalar FFmpeg pixel conversion and software MPEG-4 encoding. This avoids the reproduced pink/green stripe corruption in the bundled build, at a possible performance cost.
- Export stages its output in a private directory beside the destination, without an application-held output file handle. Existing output is replaced only after success; Windows failures include the system error.
- Remove unused project media by button, context menu or Delete in the media list. Source files are preserved. Used media must first be removed from the timeline.
- Text clips on a separate T1 layer: Unicode, multiple lines, font, size, bold, color, width, position, duration and visual fade in/out. Click or drag in the prepared preview to position the selected text.
- Brightness, contrast, saturation and hue per video/image/text clip.
- JPG/PNG import, adjustable image duration, alpha transparency and fades.
- Fade, wipe and slide transitions, with source-anchored envelopes preserved by splitting.
- Landscape 16:9 and portrait 9:16 projects, previews and 720p/1080p exports. Smartphone rotation metadata is applied automatically.
- Updated German About text, help and documentation.

## Editing and output

Two video tracks, two audio tracks and a title layer; move, trim, split, delete, duplicate, copy/paste, snapping, locks and audio mute. 100-step undo/redo. Audio gain and fades, music mixing, atomic project saves, relinking and recovery backups are supported.

MP4/MKV use **MPEG-4 Part 2 video and AAC audio**. H.264/HEVC input remains supported by the decoder; the editor does not offer H.264 output. MP3 uses LAME; WAV uses PCM. Video export offers 720p or 1080p in either orientation, 24/25/30/60 fps and three quality levels. Silence is present when the project has no sound. Some devices/platforms accept fewer MPEG-4 Part 2 files than H.264 files.

Preview is a locally prepared composition, shared with export. After edits, prepare again; frame stepping also prepares a paused still. Images and text are transparent outside their content; ordinary video is fitted with black bars. V2 overlays V1, T1 overlays V2. Overlap clips on different tracks for transitions between clips. Without another clip underneath, a transition reveals black.

## Project compatibility and limits

Projects from earlier versions open and migrate; saving writes schema 3, including opacity. Newly saved projects require version 1.0.0 or later. Keep a copy if you need the old application. Source media are never rewritten.

The editor is SDR, with no HDR color management, arbitrary video transforms, keyframes, speed changes, waveforms, proxies or 4K export. Maximum project duration is 24 hours; 10,000 clips/assets; 128 distinct render inputs. Images are limited to 8192 pixels per side, 32 megapixels and 128 MiB on import. Fonts must be installed on the current computer and are not embedded. Long projects and many high-resolution images may need substantial temporary storage. Scalar software conversion prioritizes correctness; real-time playback is not guaranteed.

## Documentation

- [German user guide](docs/USER_GUIDE_DE.md)
- [Build instructions](docs/BUILD.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Implementation and test evidence](docs/IMPLEMENTATION.md)
- [Acceptance status](docs/ACCEPTANCE.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)

Own application code: MIT. Dependencies keep their own licenses. The source ZIP includes matching Qt, FFmpeg and LAME source archives, build instructions, the LAME patch and license texts. Synthetic example media are CC0.
