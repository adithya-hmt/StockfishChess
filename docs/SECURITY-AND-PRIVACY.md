# Security and privacy model

Cerelytic Chess is designed as a local application. It has no account service, network client, analytics endpoint, advertising SDK, or cloud synchronization layer.

## Local profiles

A profile is a record inside the app sandbox containing a display name, avatar choice, rating, game counters, puzzle counters, and preferences. It is not an internet identity. Profiles never receive an email address, access token, or server identifier.

## Local PIN

The optional four-digit PIN is a convenience boundary between local profiles. The stored value is a one-way application hash rather than the plain digits, but a four-digit space is inherently small. It should not be treated as protection against a device owner, forensic access, or a compromised operating system. Android device encryption and screen locking remain the real security boundary.

## Stored data

The app stores only:

- profiles and settings;
- the active game and clocks;
- local result history and move logs;
- puzzle progress.

Reset Local Data removes all of those records. Removing the Android application also removes its sandboxed state.

## Permissions and data flow

The packaged manifest requests no Android permissions. Stockfish is launched from the APK and communicates through local pipes. Positions, hints, evaluations, games, and PIN state remain on the device. Local exports are written only to the app-specific external-files directory Android assigns to the package.

## Signing

A stable Android signing key is required for future in-place updates. The public source archive excludes private signing keys. The Fedora bundle may include the user's private local release key so their own builds remain update-compatible; that bundle should be stored privately and never committed.
