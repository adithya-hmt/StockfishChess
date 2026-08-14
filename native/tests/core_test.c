#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../core.h"

static void test_initial_board(void) {
    SfGame g;
    sf_game_reset(&g);
    assert(g.board[sf_square_to_index("e1")] == 'K');
    assert(g.board[sf_square_to_index("d8")] == 'q');
    assert(g.board[sf_square_to_index("e2")] == 'P');
    assert(g.board[sf_square_to_index("e7")] == 'p');
}

static void test_basic_and_special_moves(void) {
    SfGame g;
    sf_game_reset(&g);
    assert(sf_apply_uci(&g, "e2e4") == 1);
    assert(g.board[sf_square_to_index("e4")] == 'P');
    assert(g.board[sf_square_to_index("e2")] == 0);

    sf_game_reset(&g);
    sf_apply_uci(&g, "e1g1");
    assert(g.board[sf_square_to_index("g1")] == 'K');
    assert(g.board[sf_square_to_index("f1")] == 'R');
    assert(g.board[sf_square_to_index("h1")] == 0);

    sf_game_reset(&g);
    sf_apply_uci(&g, "e2e4");
    sf_apply_uci(&g, "a7a6");
    sf_apply_uci(&g, "e4e5");
    sf_apply_uci(&g, "d7d5");
    sf_apply_uci(&g, "e5d6");
    assert(g.board[sf_square_to_index("d6")] == 'P');
    assert(g.board[sf_square_to_index("d5")] == 0);

    memset(&g, 0, sizeof(g));
    g.board[sf_square_to_index("a7")] = 'P';
    sf_apply_uci(&g, "a7a8q");
    assert(g.board[sf_square_to_index("a8")] == 'Q');
}

static void test_uci_parsers(void) {
    char move[6];
    assert(sf_parse_bestmove("bestmove e7e5 ponder g1f3", move) == 1);
    assert(strcmp(move, "e7e5") == 0);
    assert(sf_parse_bestmove("info depth 10", move) == 0);
    assert(sf_parse_perft_move("e2e4: 1", move) == 1);
    assert(strcmp(move, "e2e4") == 0);
    assert(sf_parse_perft_move("a7a8q: 1", move) == 1);
    assert(strcmp(move, "a7a8q") == 0);
}

static void test_move_pick(void) {
    const char *legal[] = {"e2e3", "e2e4", "g1f3"};
    char move[6];
    assert(sf_pick_legal_move(legal, 3, "e2", "e4", move) == 1);
    assert(strcmp(move, "e2e4") == 0);
    assert(sf_pick_legal_move(legal, 3, "e2", "e5", move) == 0);
}

static void test_undo_plies(void) {
    SfGame g;
    sf_game_reset(&g);
    assert(sf_apply_uci(&g, "e2e4") == 1);
    assert(sf_apply_uci(&g, "e7e5") == 1);
    assert(sf_apply_uci(&g, "g1f3") == 1);
    assert(sf_apply_uci(&g, "b8c6") == 1);

    assert(sf_undo_plies(&g, 2) == 2);
    assert(strcmp(g.history, "e2e4 e7e5") == 0);
    assert(g.board[sf_square_to_index("e4")] == 'P');
    assert(g.board[sf_square_to_index("e5")] == 'p');
    assert(g.board[sf_square_to_index("g1")] == 'N');
    assert(g.board[sf_square_to_index("b8")] == 'n');

    assert(sf_undo_plies(&g, 2) == 2);
    assert(g.history_len == 0);
    assert(g.board[sf_square_to_index("e2")] == 'P');
    assert(g.board[sf_square_to_index("e7")] == 'p');
    assert(sf_undo_plies(&g, 2) == 0);
}

static void test_history_helpers(void);
static void test_fen_loading(void);
static void test_king_and_material_helpers(void);
static void test_check_detection_and_hash(void);

int main(void) {
    test_initial_board();
    test_basic_and_special_moves();
    test_uci_parsers();
    test_move_pick();
    test_undo_plies();
    test_history_helpers();
    test_king_and_material_helpers();
    test_check_detection_and_hash();
    test_fen_loading();
    puts("native core tests: PASS");
    return 0;
}

static void test_history_helpers(void) {
    SfGame g, replay;
    char move[6];
    sf_game_reset(&g);
    assert(sf_apply_uci(&g, "e2e4"));
    assert(sf_apply_uci(&g, "e7e5"));
    assert(sf_apply_uci(&g, "g1f3"));
    assert(sf_history_move_count(&g) == 3);
    assert(sf_history_get_move(&g, 1, move));
    assert(strcmp(move, "e7e5") == 0);
    assert(sf_game_from_history(&replay, g.history, 2));
    assert(replay.board[sf_square_to_index("e4")] == 'P');
    assert(replay.board[sf_square_to_index("e5")] == 'p');
    assert(replay.board[sf_square_to_index("g1")] == 'N');
}

static void test_king_and_material_helpers(void) {
    SfGame g;
    memset(&g, 0, sizeof(g));
    g.board[sf_square_to_index("e1")] = 'K';
    g.board[sf_square_to_index("e8")] = 'k';
    assert(sf_find_king(&g, 1) == sf_square_to_index("e1"));
    assert(sf_find_king(&g, 0) == sf_square_to_index("e8"));
    assert(sf_is_insufficient_material(&g));
    g.board[sf_square_to_index("c1")] = 'B';
    assert(sf_is_insufficient_material(&g));
    g.board[sf_square_to_index("b1")] = 'N';
    assert(!sf_is_insufficient_material(&g));
    g.board[sf_square_to_index("b1")] = 0;
    g.board[sf_square_to_index("a2")] = 'P';
    assert(!sf_is_insufficient_material(&g));
}

static void test_fen_loading(void) {
    SfGame g;
    assert(sf_game_load_fen(&g, "7k/5Q2/7K/8/8/8/8/8 b - - 0 1"));
    assert(g.board[sf_square_to_index("h8")] == 'k');
    assert(g.board[sf_square_to_index("f7")] == 'Q');
    assert(g.board[sf_square_to_index("h6")] == 'K');
    assert(sf_side_to_move(&g) == 0);
    assert(sf_apply_uci(&g, "h8g8"));
    assert(sf_side_to_move(&g) == 1);
}


static void test_check_detection_and_hash(void) {
    SfGame g, before;
    unsigned long long initial_hash;
    sf_game_reset(&g);
    before = g;
    initial_hash = sf_board_hash(&g);
    assert(!sf_is_in_check(&g, 1));
    assert(!sf_is_in_check(&g, 0));
    assert(sf_material_balance(&g) == 0);
    assert(sf_apply_uci(&g, "f2f3"));
    assert(sf_apply_uci(&g, "e7e5"));
    assert(sf_apply_uci(&g, "g2g4"));
    assert(sf_apply_uci(&g, "d8h4"));
    assert(sf_is_in_check(&g, 1));
    assert(!sf_is_in_check(&g, 0));
    assert(sf_board_hash(&g) != initial_hash);
    assert(sf_board_hash(&before) == initial_hash);

    memset(&g, 0, sizeof(g));
    g.board[sf_square_to_index("e1")] = 'K';
    g.board[sf_square_to_index("e8")] = 'k';
    g.board[sf_square_to_index("a8")] = 'r';
    assert(sf_is_square_attacked(&g, sf_square_to_index("a1"), 0));
    assert(!sf_is_square_attacked(&g, sf_square_to_index("b1"), 0));
}
