# Framilton Chess v3 implementation report

## Delivered scope

Framilton Chess v3 is a working offline Android application. The same native renderer used by the APK produces the repository's UI previews.

### Screens and behavior

The app includes splash and onboarding, local profile creation and PIN flows, Home, Play setup, Stockfish and pass-and-play games, puzzles, history, replay, local analysis, profile statistics, and settings.

Core behavior includes device-local profiles, optional local lock, persistent settings and active-game resume, Stockfish legal moves/best moves/hints/evaluation, background engine work, pass-and-play, offline tactics, history/replay/analysis, touch and drag movement, promotion, clocks, haptics, check/checkmate feedback, and local export.

## Isolation boundaries

- Chess rules are isolated in `core`.
- Stockfish process communication is isolated in `engine`.
- Persistence is isolated in `state`.
- Drawing assets and primitives are isolated in `ui`.
- Android lifecycle and screen coordination remain in `activity`.
- Framilton presentation is applied through the product assets, Android metadata, and `framilton_identity.h`.

## Verification layers

- Host unit tests for rules, persistence, parser, and model contracts.
- UCI integration against Stockfish.
- Android manifest and resource patch tests.
- Renderer smoke tests.
- Freestanding ARM64 compilation with warnings treated as errors.
- APK archive, signature, payload, and alignment checks.
- Repository identity guard via `scripts/check-framilton-identity.sh`.

## Product identity boundary

The Android package is `com.framilton.knight`. Framilton Chess deliberately uses a separate package namespace, persisted-state filename, signing defaults, launcher assets, and release artifact names. It does not import application data from builds distributed under a different identity.

## Engine provenance

The UI labels the local engine as `STOCKFISH`. Exact engine label, upstream reference, binary SHA-256, license SHA-256, and build date are embedded in `assets/BUILD-METADATA.txt` inside every APK. See `docs/ENGINE-PROVENANCE.md` for pinned upstream details.
