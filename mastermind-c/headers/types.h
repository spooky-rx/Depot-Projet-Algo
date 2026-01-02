#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#define CODE_LEN 4
#define MAX_COLORS 6
#define MIN_COLORS 3
#define MAX_TRIES_MIN 5
#define MAX_TRIES_MAX 30

typedef struct {
    int color_count;       // 3..6
    int max_tries;         // 5..30
    bool allow_repetition; // secret & guesses
    bool timed_mode;       // chrono par tentative
    int time_per_try_sec;  // secondes si timed_mode
} GameConfig;

typedef struct {
    char guesses[64][CODE_LEN];
    int blacks[64];
    int whites[64];
    int tries;
    char secret[CODE_LEN];
    GameConfig cfg;
    bool in_progress;
} GameState;

typedef struct {
    unsigned long games_played;
    unsigned long games_won;
    unsigned long total_tries; // somme des tentatives
    double total_time;         // somme des temps (secondes)
} Stats;

#endif
