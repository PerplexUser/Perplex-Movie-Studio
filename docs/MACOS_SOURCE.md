# Source provenance for the macOS port

Authoritative input: PerplexMovieStudio-0.1.1-dev-r2-Source(3).zip

SHA-256: `81e0137396f82a300ceec9693f136e5e6b57035758331e3ad8b138afb5044178`.

The application source and tests match the R2 GitHub snapshot before the macOS-specific changes. The supplied ZIP restores the license files and LAME patch missing from that snapshot. The five repository Qt parts unpack to the exact original qtbase source archive (SHA-256 `56001b905601bb9023d399f3ba780d7fa940f3e4861e496a7c490331f49e0b80`). `packaging/prepare_sources.py` enforces this; generated source archives include the complete file.
