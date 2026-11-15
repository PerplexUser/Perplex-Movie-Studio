# Changelog

## 1.0.0

- Fixed the title editor resetting its cursor after delayed saves. Typing, selection and text undo remain intact.
- Added clip opacity (0–100%) to Picture, shared by preview and export; combines with PNG alpha, fades, wipe and slide.
- Persisted opacity in project schema 3 while retaining read support for schema 1/2.
- Unified About, package and executable versions at 1.0.0 and removed the development label.


## 0.1.1-dev-r2 — macOS port

- Added native ARM64/Intel app bundles, icon and Finder project-file opening.
- Added macOS FFmpeg library discovery while retaining the software pixel policy.
- Added bundled LGPL media builds, dependency/architecture checks and relocated-runtime tests.
- Added reproducible ZIP/DMG packaging and GitHub Actions; archives require passing native tests.
- Kept the application version and requested About text unchanged.
- Default signing is ad-hoc; Developer ID notarization and manual Mac acceptance remain separate.

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
