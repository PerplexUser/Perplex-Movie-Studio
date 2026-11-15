# Perplex Movie Studio

## Scope and architecture
- The attached requirements in `docs/specification/` define the product.
- Develop milestones M0 through M6 sequentially. Record requirements, tests and actual results in `docs/IMPLEMENTATION.md`.
- C++20 domain code has no Qt, UI or FFmpeg dependencies. UI calls atomic domain commands.
- Timeline time is signed 64-bit microseconds. Frame positions are calculated from rational FPS, never accumulated floats. Intervals are half-open.
- Every completed edit is one undo operation; reject invalid changes without modifying the project. Locked tracks cannot be edited.
- Use one filter-graph builder for cached preview and final export. Keep all media work outside the UI thread using asynchronous processes.
- Pass process arguments as a list, never via a shell. Only local file/pipe protocols are allowed. Bound input, output and job sizes.
- Save projects and outputs atomically. Do not modify source media. Protect existing destinations on failure and cancellation.
- All visible UI text must be translatable. Default language is English; German is persistent.

## Licensing and verification
- Original application code is MIT. Do not copy third-party proprietary code or assets.
- Dynamically link LGPL-compatible Qt modules. Do not add GPL/nonfree components to a distributed FFmpeg build.
- Do not claim a release, a passing test or Windows compatibility without evidence. Windows VM, performance and distribution-license gates remain explicit.
- No inactive feature may look operational. Add meaningful regression tests for editing, persistence and rendering changes.
- No network access, telemetry, account or uploads in the application.
