# Framilton Chess security and privacy model

Framilton Chess is designed as a local application. It has no account service, network client, analytics endpoint, advertising SDK, or cloud synchronization layer.

## Local profiles

A profile is a record inside the app sandbox containing a display name, avatar choice, rating, game counters, puzzle counters, and preferences. It is not an internet identity and never receives an email address, access token, or server identifier.

## Local PIN

The optional four-digit PIN is a convenience boundary between local profiles. The stored value is a one-way application hash rather than plain digits, but a four-digit space is inherently small. Android device encryption and screen locking remain the real security boundary.

## Stored data

The app stores profiles/settings, active-game state and clocks, local result history and move logs, and puzzle progress in the `com.framilton.knight` application sandbox. The persisted state filename is `framilton-v3.state`.

Reset Local Data removes those records. Removing the Android application also removes its sandboxed state.

## Permissions and data flow

The packaged manifest requests no Android permissions. Stockfish is launched from the APK and communicates through local pipes. Positions, hints, evaluations, games, and PIN state remain on the device. Local exports are written only to the app-specific external-files directory Android assigns to the package.

## Signing

A stable Android signing key is required for future in-place Framilton updates. Public source excludes private signing keys. Keep release keystores private and backed up.
