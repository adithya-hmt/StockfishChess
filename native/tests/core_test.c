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

int main(void) {
    test_initial_board();
    test_basic_and_special_moves();
    test_uci_parsers();
    test_move_pick();
    puts("native core tests: PASS");
    return 0;
}
