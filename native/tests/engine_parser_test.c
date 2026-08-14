#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../engine.h"
int main(void) {
    CerAnalysis a;
    memset(&a, 0, sizeof(a));
    assert(cer_parse_info_line("info depth 18 seldepth 24 score cp -42 nodes 10 pv e2e4 e7e5", &a));
    assert(a.depth == 18);
    assert(a.eval_cp == -42);
    assert(strcmp(a.pv, "e2e4 e7e5") == 0);
    assert(cer_parse_info_line("info depth 22 score mate 3 pv h5f7", &a));
    assert(a.mate_in == 3);
    puts("engine parser tests: PASS");
    return 0;
}
