# Perplex Movie Studio

**0.1.1-dev · Revision 2** — local Windows video editor by Michael Lappenbusch · www.perplex.click.

## Start

Run `PerplexMovieStudio-0.1.1-dev-r2-Windows-x64-Setup.exe`. Installation is for the current user and creates a Start menu shortcut. Alternatively extract the entire portable ZIP and run `PerplexMovieStudio.exe`. Keep the EXE, DLLs, plugins, translations and licenses together. No Python, SDK, account or internet connection is needed.

Windows 10 22H2 / Windows 11 x64 are the target systems. This unsigned development build was cross-compiled for Windows; execution on a clean Windows machine and performance acceptance remain open. The About dialog intentionally says **Version 0.1.1-dev**. Windows file metadata and package filenames identify Revision 2 (0.1.1.2).

The initial language is English. Choose **Settings → Language → Deutsch** for German; the choice is remembered.

## Revision 2

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

MP4/MKV use **MPEG-4 Part 2 video and AAC audio**. H.264/HEVC input remains supported by the decoder; this revision does not offer H.264 output. MP3 uses LAME; WAV uses PCM. Video export offers 720p or 1080p in either orientation, 24/25/30/60 fps and three quality levels. Silence is present when the project has no sound. Some devices/platforms accept fewer MPEG-4 Part 2 files than H.264 files.

Preview is a locally prepared composition, shared with export. After edits, prepare again; frame stepping also prepares a paused still. Images and text are transparent outside their content; ordinary video is fitted with black bars. V2 overlays V1, T1 overlays V2. Overlap clips on different tracks for transitions between clips. Without another clip underneath, a transition reveals black.

## Project compatibility and limits

Schema 1 projects open and migrate; saving writes schema 2. Older application builds cannot open newly saved schema 2 projects. Keep a copy if you need the old application. Source media are never rewritten.

The editor is SDR, with no HDR color management, arbitrary video transforms, keyframes, speed changes, waveforms, proxies or 4K export. Maximum project duration is 24 hours; 10,000 clips/assets; 128 distinct render inputs. Images are limited to 8192 pixels per side, 32 megapixels and 128 MiB on import. Fonts must be installed on the current computer and are not embedded. Long projects and many high-resolution images may need substantial temporary storage. Scalar software conversion prioritizes correctness; real-time playback is not guaranteed.

## Documentation

- [German user guide](docs/USER_GUIDE_DE.md)
- [Build instructions](docs/BUILD.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Implementation and test evidence](docs/IMPLEMENTATION.md)
- [Acceptance status](docs/ACCEPTANCE.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)

Own application code: MIT. Dependencies keep their own licenses. The source ZIP includes matching Qt, FFmpeg and LAME source archives, build instructions, the LAME patch and license texts. Synthetic example media are CC0.
