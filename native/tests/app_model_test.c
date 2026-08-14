#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../app_model.h"

static void test_defaults_and_profiles(void) {
    CerData d;
    cer_data_defaults(&d);
    assert(cer_data_validate(&d));
    assert(d.profile_count == 1);
    assert(strcmp(d.profiles[0].name, "White Knight") == 0);
    assert(d.settings.haptics == 1);
    assert(cer_add_profile(&d, "Tactician", 2, 1) == 1);
    d.checksum = cer_data_checksum(&d);
    assert(cer_data_validate(&d));
}

static void test_pin_hash(void) {
    assert(cer_pin_hash("2468") == cer_pin_hash("2468"));
    assert(cer_pin_hash("2468") != cer_pin_hash("2469"));
    assert(cer_pin_hash("") != 0);
}

static void test_save_load_and_corruption(void) {
    const char *path = "/tmp/cerelytic-v3-model.dat";
    CerData a, b;
    FILE *f;
    unlink(path);
    cer_data_defaults(&a);
    a.onboarding_complete = 1;
    a.profiles[0].rating = 1234;
    assert(cer_data_save(&a, path));
    memset(&b, 0, sizeof(b));
    assert(cer_data_load(&b, path));
    assert(b.onboarding_complete == 1);
    assert(b.profiles[0].rating == 1234);
    f = fopen(path, "r+b");
    assert(f);
    fputc(0, f);
    fclose(f);
    assert(!cer_data_load(&b, path));
    assert(b.profile_count == 1);
    unlink(path);
}

static void test_history_and_stats(void) {
    CerData d;
    cer_data_defaults(&d);
    cer_record_game(&d, "e2e4 e7e5 g1f3", CER_RESULT_WIN, 2, 0, 0);
    assert(d.history_count == 1);
    assert(d.history[0].move_count == 3);
    assert(d.profiles[0].wins == 1);
    assert(d.profiles[0].rating == 812);
    cer_record_game(&d, "d2d4 d7d5", CER_RESULT_LOSS, 3, 0, 0);
    assert(d.history_count == 2);
    assert(d.history[0].result == CER_RESULT_LOSS);
    assert(d.history[1].result == CER_RESULT_WIN);
    assert(d.profiles[0].losses == 1);
}

int main(void) {
    test_defaults_and_profiles();
    test_pin_hash();
    test_save_load_and_corruption();
    test_history_and_stats();
    puts("app model tests: PASS");
    return 0;
}
