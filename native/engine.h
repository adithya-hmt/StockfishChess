#ifndef CERELYTIC_ENGINE_H
#define CERELYTIC_ENGINE_H
#include "platform.h"
#include "core.h"

typedef struct {
    int in_fd;
    int out_fd;
    pid_t pid;
    int ready;
    int level;
} CerEngine;

typedef struct {
    int in_check;
    int eval_cp;
    int mate_in;
    int depth;
    char bestmove[6];
    char pv[64];
} CerAnalysis;

void cer_engine_init(CerEngine *engine);
int cer_engine_start(CerEngine *engine);
void cer_engine_stop(CerEngine *engine);
int cer_engine_set_level(CerEngine *engine, int level);
int cer_engine_legal_moves(CerEngine *engine, const SfGame *game, char legal[][6], int capacity);
int cer_engine_inspect(CerEngine *engine, const SfGame *game, int *in_check);
int cer_engine_bestmove(CerEngine *engine, const SfGame *game, int think_ms, char out[6]);
int cer_engine_analyze(CerEngine *engine, const SfGame *game, int think_ms, CerAnalysis *analysis);
int cer_parse_info_line(const char *line, CerAnalysis *analysis);

#endif
