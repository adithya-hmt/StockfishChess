# Changelog

## 3.0.0 — 2026-08-14

Framilton Chess v3 replaces the single-board prototype with a complete local-first chess application.

### Added

- Branded Home dashboard and five-destination navigation shell.
- Stockfish and pass-and-play setup with side, strength, clock, hint, and legal-marker choices.
- Device-local profiles, avatars, ratings, records, puzzle progress, profile switching, and optional PIN lock.
- Five offline tactical puzzles.
- Persistent active-game resume, local game history, replay, and analysis.
- Tap and drag input, animated moves, promotion choices, haptics, board flip, clocks, hints, undo, rematch, resign, and local export.
- Amber check state and dedicated red checkmate presentation.
- Framilton identity layer, launcher treatment, controls, and Staunton piece set.
- Dedicated worker thread for Stockfish move, hint, and analysis searches.
- Fedora one-command setup/build/install script and signed GitHub Actions release workflow.

### Changed

- Android package is `com.framilton.knight` for a clean Framilton identity boundary.
- APK name is `FramiltonChess-v3-offline-arm64.apk`.
- Build, signing, release, and local cache defaults use Framilton naming.

### Security and privacy

- No Android permissions.
- No network client, analytics, advertising, or account service.
- Public source packages exclude private signing keys.
