# Changelog

## 3.0.0 — 2026-08-14

Cerelytic Chess v3 replaces the single-board prototype with a complete local-first chess application.

### Added

- Branded Home dashboard and five-destination navigation shell.
- Stockfish and pass-and-play setup with side, strength, clock, hint, and legal-marker choices.
- Device-local profiles, avatars, ratings, records, puzzle progress, profile switching, and optional PIN lock.
- Five offline tactical puzzles.
- Persistent active-game resume, local game history, replay, and analysis.
- Tap and drag input, animated moves, promotion choices, haptics, board flip, clocks, hints, undo, rematch, resign, and local export.
- Amber check state and dedicated red checkmate presentation.
- Original Cerelytic mark, launcher icon, avatars, controls, and Staunton piece set.
- Dedicated worker thread for Stockfish move, hint, and analysis searches.
- Fedora one-command setup/build/install script and signed GitHub Actions release workflow.

### Changed

- Android package moved to `com.cerelytic.knight` so v3 can install beside the legacy prototype.
- APK name is now `CerelyticChess-v3-offline-arm64.apk`.

### Security and privacy

- No Android permissions.
- No network client, analytics, advertising, or account service.
- Public source packages exclude private signing keys.
