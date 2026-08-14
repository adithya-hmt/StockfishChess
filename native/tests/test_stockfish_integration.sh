#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HOST_ENGINE="${STOCKFISH_HOST_BIN:-}"

[[ -n "$HOST_ENGINE" && -x "$HOST_ENGINE" ]] || {
  cat >&2 <<'MSG'
Set STOCKFISH_HOST_BIN to an executable host Stockfish binary.
Example:
  STOCKFISH_HOST_BIN=/usr/bin/stockfish native/tests/test_stockfish_integration.sh
MSG
  exit 2
}

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
cp "$HOST_ENGINE" "$work/libstockfish.so"
cat > "$work/wrapper.c" <<'C'
#include <string.h>
#include "engine.h"

static CerEngine engine;
static SfGame game;
static int started;

__attribute__((visibility("default"))) int cer_test_start(void) {
    cer_engine_init(&engine);
    started = cer_engine_start(&engine);
    return started;
}
__attribute__((visibility("default"))) int cer_test_set_fen(const char *fen) { return sf_game_load_fen(&game, fen); }
__attribute__((visibility("default"))) int cer_test_reset(void) { sf_game_reset(&game); return 1; }
__attribute__((visibility("default"))) int cer_test_legal_count(void) {
    char legal[256][6]; return cer_engine_legal_moves(&engine, &game, legal, 256);
}
__attribute__((visibility("default"))) int cer_test_is_legal(const char *move) {
    char legal[256][6];
    int i, count = cer_engine_legal_moves(&engine, &game, legal, 256);
    if (count < 0) return -1;
    for (i = 0; i < count; ++i) if (strcmp(legal[i], move) == 0) return 1;
    return 0;
}
__attribute__((visibility("default"))) int cer_test_bestmove(int think_ms, char out[6]) {
    memset(out, 0, 6); return cer_engine_bestmove(&engine, &game, think_ms, out);
}
__attribute__((visibility("default"))) int cer_test_analyze(int think_ms, CerAnalysis *out) {
    return cer_engine_analyze(&engine, &game, think_ms, out);
}
__attribute__((visibility("default"))) void cer_test_stop(void) {
    if (started) cer_engine_stop(&engine);
    started = 0;
}
C

clang -std=c11 -Wall -Wextra -Werror -fPIC -shared -pthread \
  -I"$ROOT/native" \
  "$work/wrapper.c" "$ROOT/native/engine.c" "$ROOT/native/core.c" \
  -o "$work/libsf_chess.so"

python3 - "$work/libsf_chess.so" <<'PY'
import ctypes
import sys

lib = ctypes.CDLL(sys.argv[1])
lib.cer_test_start.restype = ctypes.c_int
lib.cer_test_set_fen.argtypes = [ctypes.c_char_p]
lib.cer_test_set_fen.restype = ctypes.c_int
lib.cer_test_reset.restype = ctypes.c_int
lib.cer_test_legal_count.restype = ctypes.c_int
lib.cer_test_is_legal.argtypes = [ctypes.c_char_p]
lib.cer_test_is_legal.restype = ctypes.c_int
lib.cer_test_bestmove.argtypes = [ctypes.c_int, ctypes.c_char_p]
lib.cer_test_bestmove.restype = ctypes.c_int

class Analysis(ctypes.Structure):
    _fields_ = [
        ("in_check", ctypes.c_int),
        ("eval_cp", ctypes.c_int),
        ("mate_in", ctypes.c_int),
        ("depth", ctypes.c_int),
        ("bestmove", ctypes.c_char * 6),
        ("pv", ctypes.c_char * 64),
    ]

lib.cer_test_analyze.argtypes = [ctypes.c_int, ctypes.POINTER(Analysis)]
lib.cer_test_analyze.restype = ctypes.c_int

puzzles = [
    (b"rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq g3 0 2", b"d8h4"),
    (b"r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4", b"h5f7"),
    (b"7k/5Q2/6K1/8/8/8/8/8 w - - 0 1", b"f7g7"),
    (b"7k/8/6K1/8/8/8/8/5R2 w - - 0 1", b"f1f8"),
    (b"r3k3/8/8/1N6/8/8/8/4K3 w - - 0 1", b"b5c7"),
]

assert lib.cer_test_start() == 1, "Stockfish failed to start"
try:
    assert lib.cer_test_reset() == 1
    assert lib.cer_test_legal_count() == 20

    best = ctypes.create_string_buffer(6)
    assert lib.cer_test_bestmove(200, best) == 1
    assert lib.cer_test_is_legal(best.value) == 1

    for fen, solution in puzzles:
        assert lib.cer_test_set_fen(fen) == 1
        assert lib.cer_test_is_legal(solution) == 1, (fen, solution)

    assert lib.cer_test_set_fen(puzzles[1][0]) == 1
    analysis = Analysis()
    assert lib.cer_test_analyze(250, ctypes.byref(analysis)) == 1
    analyzed = bytes(analysis.bestmove).split(b"\0", 1)[0]
    assert analysis.depth > 0
    assert lib.cer_test_is_legal(analyzed) == 1
finally:
    lib.cer_test_stop()

print("Stockfish process integration: PASS")
PY
