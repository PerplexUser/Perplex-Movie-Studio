# Contributing

Read AGENTS.md and docs/BUILD.md first. Describe the affected requirement and
acceptance test before changing behavior. Keep domain code independent of Qt.
Provide tests for transactional edits and persistence; render tests should inspect
actual samples/streams rather than just checking the generated command string.
Use Qt translatable strings and update the German catalog. Run CTest and the
translation checker. Never mark a Windows test as passing based on a Linux run.
Do not add GPL/nonfree runtime dependencies or proprietary assets. No telemetry.
Contributions to original application code are under MIT.
