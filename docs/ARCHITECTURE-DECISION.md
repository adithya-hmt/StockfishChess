# V3 architecture decision

The first v3 plan proposed a conventional Kotlin/Compose rewrite. The delivered v3 keeps the freestanding NativeActivity architecture and deepens it into isolated rules, engine, persistence, drawing, and application modules.

## Why the implementation changed

- It produces a fully offline APK without Gradle dependency resolution or Android Studio.
- The existing ARM64/native Stockfish packaging path remains small and auditable.
- Rules, persistence, UCI parsing, and rendering can all run as host tests on Fedora and in GitHub Actions.
- The app requests no Android permissions. Local PIN locking therefore replaces BiometricPrompt, which would add a platform permission and a system-auth dependency.
- The same renderer used in the APK generates the checked-in screen gallery, avoiding mockups that quietly diverge from the shipping interface.

## Delivered boundaries

- `core`: chess position and move rules
- `engine`: Stockfish process and UCI protocol
- `state`: profiles, settings, active game, puzzles, and history
- `ui`: brand, pieces, icons, text, and drawing primitives
- `activity`: lifecycle, navigation, input, clocks, worker coordination, and screen behavior

## Deliberate v3.0 limits

- Phone-first portrait UI rather than a separate tablet layout.
- Device-local PIN rather than biometric or remote identity.
- App-specific local export rather than a full document-picker import/export flow.
- Five bundled tactical puzzles rather than a large licensed puzzle database.
- Native accessibility is limited compared with a full Compose semantics tree.

These are product boundaries, not hidden unfinished placeholders. The implemented surfaces are listed in `docs/IMPLEMENTATION.md` and tested by the repository. Humans have invented enough software that says “complete” while keeping twelve secret TODO lists; this project does not need a thirteenth.
