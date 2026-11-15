# Architecture — 0.1.1-dev Revision 2

## Editing and persistence

The domain is C++20 without Qt/FFmpeg. Integer microseconds and rational frame math avoid accumulated drift; intervals are half-open. `Editor` validates atomic snapshot transactions, keeps 100 undo steps and respects track locks. Media/Image/Title asset kinds and clip-level visual/title settings are explicit value types. Split preserves source-anchored audio and visual envelopes. Trimming resets the envelope to the new source interval and clamps fade lengths.

`ProjectStore` writes schema 2 using QSaveFile without a direct-write fallback; schema 1 is migrated on read. JSON is limited to 16 MiB, values and enums are validated, and relative media paths have an absolute fallback. Generated text does not require a source file. Recovery backups and relinking remain local.

## Media and composition

Media probing/decode uses asynchronous QProcess with argument lists and no shell. Only file/pipe protocols are allowed. Images use bounded QImageReader tasks with automatic orientation. Image/text preparation runs with QtConcurrent; QPainter renders fonts and images to an RGBA canvas. The temporary raw frames retain straight alpha and are looped by FFmpeg, avoiding reliance on its PNG decoder. Output dimensions, image size, render input count and diagnostics are bounded.

One graph builds preview and export: normalize timestamps → source trim → project frame rate → aspect fit/pad → color adjustments → source-anchored transition → timeline placement → ordered overlays. RGB lookup tables implement brightness/contrast; hue/saturation use alpha-preserving conversion. Fade modifies alpha; wipe clips alpha; slide changes overlay position. Output is explicit yuv420p. Video padding is black; image/text padding is transparent. T1 overlays V2, which overlays V1.

Audio normalizes timestamps, resamples to 48 kHz stereo, applies original envelopes and gain, trims and delays in samples, then mixes over silence with a latency-compensated limiter. Splitting retains fade progression.

## Color policy and preview

The supplied FFmpeg build disables external x86 assembly but still has CPU-dispatched code. Its optimized pixel path produced reproducible chroma corruption. Revision 2 forces `-cpuflags 0` for probe/decode/render jobs and calls `av_force_cpu_flags(0)` in the libavutil used by Qt before decoding frames. A test checks `av_get_cpu_flags()==0`. No dependency binary is patched.

Qt uses its FFmpeg backend with hardware decoding and hardware texture conversion disabled. A QVideoSink delivers software frames, converted to QImage and painted by QPainter. Playback and seeking use the prepared composition and Qt's clock. A guarded initial-load seek displays a paused still without repeatedly seeking on status signals. The last valid image remains visible while a replacement preview is prepared. This policy may reduce performance; Windows runtime performance is an open acceptance item.

## Output and packaging

RenderJob prepares assets/graph asynchronously, discovers the required encoders and selects software MPEG-4 Part 2 + AAC (MP4/MKV), LAME (MP3) or PCM (WAV). A private temporary directory is created in the destination's parent. FFmpeg owns its output handle; after it exits successfully, MoveFileExW (Windows) or rename/link (POSIX) promotes the completed file. The old output survives cancellation/failure. Temporary graph handles are released before directory cleanup. Windows promotion errors include the system code/message.

The application uses shared, replaceable Qt and FFmpeg DLLs. The Windows package statically validates x64 PE imports, including delayed imports and imported symbol names/ordinals against shipped DLL exports. The NSIS installer is per-user and uninstalls only its explicit installed-file list. These checks do not replace Windows execution testing.

## Deliberate limits

Preview requires preparation after edits. Portrait and landscape are aspect-preserving fits, not cropping controls. Text size is relative to project height; font availability depends on the local computer. No arbitrary transform/keyframe/proxy/HDR engine is claimed. Network, accounts, telemetry and updater are absent.
