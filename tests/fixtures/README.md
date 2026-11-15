# Synthetic media fixtures

`h264-aac.mp4` and `vp9-opus.mkv` are one-second synthetic FFmpeg testsrc2 + sine
signals generated for this project on 2026-11-12. No third-party footage, music,
fonts, logos or personal media are used. The generated media are dedicated to the
public domain under CC0-1.0, to the extent any rights arise.

These two immutable codec fixtures were generated using the host-provided QA
encoder (FFmpeg 7.1.5, libx264 / libvpx-vp9 with AAC / Opus). That host encoder and
its libraries are not included, linked or required by the application or its
test runner. Tests only decode these files using the application's LGPL build.
Additional MPEG-4, MPEG-1/2, MP2/MP3 and PCM fixtures are generated at test time
with the LGPL tools under test.

Command shape:
```
ffmpeg -f lavfi -i testsrc2=size=160x90:rate=30:duration=1 \
  -f lavfi -i sine=frequency=440:sample_rate=48000:duration=1 \
  -c:v ENCODER -c:a AUDIO -pix_fmt yuv420p -shortest OUTPUT
```

CC0 text: https://creativecommons.org/publicdomain/zero/1.0/legalcode
