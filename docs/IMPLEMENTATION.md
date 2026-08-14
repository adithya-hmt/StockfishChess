# Cerelytic Chess v3 implementation report

## Delivered scope

Cerelytic Chess v3 is a working offline Android application, not a static mockup. The same C renderer used by the APK produced the screenshots in this repository.

### Screens

1. Splash
2. Three-page onboarding
3. Local profile creation
4. Local PIN setup
5. Local PIN unlock
6. Home
7. Play setup
8. Stockfish and pass-and-play game
9. Puzzle library
10. Puzzle game
11. History
12. Replay
13. Local analysis
14. Profile and statistics
15. Settings

### Behavior

- Device-local profiles and profile switching
- Optional four-digit local lock
- Persistent settings and active-game resume
- Stockfish legal moves, best move, hint, and evaluation
- Engine move, hint, and analysis searches run on a dedicated worker thread so the Android looper stays responsive
- Pass-and-play
- Five offline tactical positions
- Game results and local history
- Replay and analysis navigation
- Touch and drag movement
- Promotion overlay
- Clock updates driven by a native looper timer
- Haptic feedback through Android View APIs
- Check amber and checkmate red result sequence
- Local export to the app-specific external files directory

## Isolation boundaries

- Chess rules are isolated in `core`.
- Stockfish process communication is isolated in `engine`.
- Persistence is isolated in `state`.
- Drawing assets and primitives are isolated in `ui`.
- Android lifecycle and screen coordination remain in `activity`.

This keeps engine failures from becoming renderer failures, and lets rules/persistence run as normal host tests. Miraculously, separation of concerns remains useful even when the concerns are written in C.

## Verification layers

- Host unit tests for rules, persistence, parser, and model contracts
- Real UCI process integration against a host Stockfish binary, including legal-move and packaged-puzzle checks
- Binary Android manifest and resource-patch tests
- Host rendering smoke tests for all fifteen application screens plus the dedicated checkmate state
- Freestanding ARM64 compilation with warnings treated as errors
- 16 KiB ELF page-alignment flags
- APK archive integrity and required-entry checks
- APK signature verification
- Embedded Stockfish payload comparison against the verified source artifact

## Explicit limits

- Local PIN is not cloud authentication and is not intended as cryptographic protection against a device owner.
- Export is a local UCI move log with game metadata rather than full SAN annotation.
- No Android emulator or physical phone is available in the build runtime, so automated device-level interaction tests are not part of the repository. The Fedora script can install the finished APK on an authorized device for final hardware testing.

## Engine provenance

The application UI deliberately says `STOCKFISH` rather than hard-coding a release number. The exact engine label, upstream reference, binary SHA-256, license SHA-256, and build date are embedded in `assets/BUILD-METADATA.txt` inside every APK. The standalone offline APK in this delivery uses the fully identified development build `5062aee519a1ba262d472d8ab139851ced56573e`; its exact source is distributed beside the APK. GitHub Actions and a networked Fedora build pin the official stable `sf_18` source archive instead.

## Legacy prototype migration

V3 uses the package ID `com.cerelytic.knight`. The earlier prototype used `com.local.sfchessapp` and was signed with a different temporary key. A package rename lets both builds install side-by-side, avoiding Android's `INSTALL_FAILED_UPDATE_INCOMPATIBLE` error and preserving the legacy app until the user deliberately removes it. Local prototype data is not imported because Android sandboxes the two packages separately.
