#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../state.h"

int main(void) {
    CerPersisted a, b;
    CerHistory h;
    int i;
    const char *dir = "/tmp";
    unlink("/tmp/framilton-v3.state");
    unlink("/tmp/framilton-v3.state.tmp");
    cer_state_defaults(&a);
    assert(a.active_profile == -1);
    assert(a.haptics_enabled == 1);
    assert(cer_profile_add(&a, "ADI", 2, 1) == 0);
    assert(strcmp(cer_active_profile(&a)->name, "ADI") == 0);
    assert(cer_pin_hash("1234") == cer_pin_hash("1234"));
    assert(cer_pin_hash("1234") != cer_pin_hash("4321"));
    memset(&h, 0, sizeof(h));
    for (i = 0; i < CER_MAX_HISTORY + 3; ++i) {
        h.result = i;
        strcpy(h.moves, "e2e4 e7e5");
        cer_history_add(&a, &h);
    }
    assert(a.history_count == CER_MAX_HISTORY);
    assert(a.history[0].result == CER_MAX_HISTORY + 2);
    assert(cer_state_save(dir, &a));
    assert(cer_state_load(dir, &b));
    assert(b.profile_count == 1);
    assert(strcmp(b.profiles[0].name, "ADI") == 0);
    assert(b.history_count == CER_MAX_HISTORY);
    unlink("/tmp/framilton-v3.state");
    puts("state tests: PASS");
    return 0;
}
