# Cerelytic Chess Brand Guidelines

## Brand premise

Cerelytic Chess is a private, local-first chess system. The identity combines a geometric knight with a circular analysis lens and a compact `C`. It should feel precise, calm, technical, and tactile rather than casino-like or noisy.

## Naming

- Product: **Cerelytic Chess**
- Short mark: **Cerelytic**
- Release descriptor: **Local Stockfish Chess**
- Never use the Chess.com name, pawn mark, wordmark, or copied interface artwork.

## Core colors

| Token | Hex | Use |
|---|---:|---|
| Deep | `#0C0E0D` | Splash and modal background |
| Background | `#121412` | Main app shell |
| Surface | `#1F231F` | Cards and player rows |
| Surface 2 | `#282D28` | Controls and elevated states |
| Cerelytic Green | `#70B84A` | Primary actions and active navigation |
| Mint | `#76D8A7` | Positive feedback and engine-ready states |
| Move Gold | `#DDBE50` | Previous move and neutral achievement |
| Check Amber | `#F29D49` | Check and caution |
| Mate Red | `#E5484D` | Checkmate, timeout losses, destructive confirmation |
| Ivory | `#EEEBD2` | Light board squares |
| Moss | `#789B59` | Dark board squares |

Mate Red is reserved. Ordinary errors should not turn the entire interface into a fire alarm.

## Typography

Use a sturdy geometric sans-serif. Interface labels are uppercase at small sizes. Titles use strong weight with slightly expanded tracking. Body copy remains sentence case when space permits. The native build embeds its own anti-aliased raster atlas, so the design does not depend on a downloaded font.

## Mark clear space

Keep clear space equal to one quarter of the outer mark width on every side. Do not stretch, rotate, outline with arbitrary colors, add gradients, or place it over a busy board position.

## Motion and haptics

- Piece travel: roughly 110 ms, shortened when Reduce Motion is enabled.
- Selection: system keyboard-tap haptic.
- Legal move: restrained confirmation haptic.
- Illegal move: short rejection haptic.
- Checkmate: one decisive confirmation/rejection event, never a repeating vibration.

## Chess pieces

The piece set is an original simplified Staunton family. Keep silhouettes consistent, preserve wide bases for small screens, and avoid photographic or copied commercial piece artwork.
