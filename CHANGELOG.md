# Changelog

## 0.1.1-dev — Revision 2

- Corrected reproducible pixel corruption by disabling faulty optimized CPU paths in both FFmpeg jobs and Qt playback. Hardware decode and texture conversion are disabled; software frames are painted by QPainter.
- Standardized MP4/MKV export on MPEG-4 Part 2 + AAC software encoding.
- Fixed Windows export promotion by eliminating the application-held temporary output handle; preserve the previous destination and report the Windows error on failure.
- Added unused-media removal, including a guard for media still used on the timeline.
- Added editable text overlays, placement by mouse or percentage, duration, typography and visual fades.
- Added JPG/PNG images with transparency, timed display and transitions.
- Added brightness, contrast, saturation and hue controls.
- Added portrait projects/preview/export and fade/wipe/slide transitions.
- Extended project format to schema 2 with schema 1 migration.
- Added full-frame color, feature and application-workflow regression tests; exact German About text and updated help.

The About version stays 0.1.1-dev as requested; the package/resource revision is 0.1.1.2. Windows runtime acceptance and performance tests remain open.
