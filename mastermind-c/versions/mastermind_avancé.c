/*
  Mastermind console — monolithique, compilable en une fois (C11)
  - Garde les mêmes “grosses” fonctions et l’IA minimax
  - Ajoute les fonctions manquantes: generate_secret_ai, print_rules, resume_game
  - Corrige l’UX (menu + prompts)
  - Corrige la logique save/quit (commande dans la saisie du guess)
  - Durcit load_game (bornes / validations pour éviter l’out-of-bounds)
  - Stocke guesses/secret avec '\0' (CODE_LEN+1) pour pouvoir les afficher en %s sans danger
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>

/* Paramètres “globaux” du jeu */
#define CODE_LEN 4
#define MAX_COLORS 6
#define MIN_COLORS 3
#define MAX_TRIES_MIN 5
#define MAX_TRIES_MAX 30
#define MAX_HISTORY 64

/* Palette */
static const char GLOBAL_COLOR_SET[MAX_COLORS]   = { 'R','G','B','Y','O','P' };
static const char *GLOBAL_COLOR_NAMES[MAX_COLORS]= { "Rouge","Vert","Bleu","Jaune","Orange","Violet" };

/* -------------------------
   Structures de données
   ------------------------- */

typedef struct {
    int color_count;       // 3..6
    int max_tries;         // 5..30
    bool allow_repetition; // doublons autorisés
    bool timed_mode;       // chrono strict
    int time_per_try_sec;  // limite par tentative si timed_mode
} GameConfig;

typedef struct {
    char guesses[MAX_HISTORY][CODE_LEN + 1]; // +1 pour '\0'
    int blacks[MAX_HISTORY];
    int whites[MAX_HISTORY];
    int tries;
    char secret[CODE_LEN + 1]; // +1 pour '\0'
    GameConfig cfg;
    bool in_progress;
} GameState;

typedef struct {
    unsigned long games_played;
    unsigned long games_won;
    unsigned long total_tries;
    double total_time;
} Stats;

/* -------------------------
   Prototypes
   ------------------------- */

void generate_secret_ai(char secret[CODE_LEN + 1], int color_count, bool allow_repetition);
void print_rules(void);
void resume_game(Stats *st);

/* -------------------------
   Presets / configuration
   ------------------------- */

static void set_default_config(GameConfig *cfg) {
    cfg->color_count = 6;
    cfg->max_tries = 10;
    cfg->allow_repetition = false;
    cfg->timed_mode = false;
    cfg->time_per_try_sec = 0;
}

static void set_preset_easy(GameConfig *cfg) {
    cfg->color_count = 3;
    cfg->max_tries = 20;
    cfg->allow_repetition = true;
    cfg->timed_mode = false;
    cfg->time_per_try_sec = 0;
}
static void set_preset_intermediate(GameConfig *cfg) {
    cfg->color_count = 4;
    cfg->max_tries = 15;
    cfg->allow_repetition = true;
    cfg->timed_mode = false;
    cfg->time_per_try_sec = 0;
}
static void set_preset_hard(GameConfig *cfg) {
    cfg->color_count = 5;
    cfg->max_tries = 10;
    cfg->allow_repetition = false;
    cfg->timed_mode = true;
    cfg->time_per_try_sec = 60;
}
static void set_preset_expert(GameConfig *cfg) {
    cfg->color_count = 6;
    cfg->max_tries = 5;
    cfg->allow_repetition = false;
    cfg->timed_mode = true;
    cfg->time_per_try_sec = 45;
}

/* -------------------------
   Entrées utilisateur
   ------------------------- */

static bool read_line(char *buffer, size_t buflen) {
    if (!buffer || buflen == 0) return false;
    if (fgets(buffer, (int)buflen, stdin) == NULL) return false;
    size_t n = strlen(buffer);
    if (n > 0 && buffer[n-1] == '\n') buffer[n-1] = '\0';
    return true;
}

static void trim_spaces(char *s) {
    if (!s) return;
    // trim left
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i > 0) memmove(s, s + i, strlen(s + i) + 1);
    // trim right
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) {
        s[n-1] = '\0';
        n--;
    }
}

static bool equals_ignore_case(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static bool is_valid_color_char(char c, int color_count) {
    c = (char)toupper((unsigned char)c);
    for (int i = 0; i < color_count; i++) {
        if (GLOBAL_COLOR_SET[i] == c) return true;
    }
    return false;
}

static bool has_no_repetition(const char code[], int len) {
    for (int i = 0; i < len; i++)
        for (int j = i+1; j < len; j++)
            if (code[i] == code[j]) return false;
    return true;
}

/*
  parse_guess:
  - garde uniquement les lettres
  - vérifie 4 lettres valides
  - applique répétition ON/OFF
*/
static bool parse_guess(const char *line, char out_code[CODE_LEN + 1],
                        int color_count, bool allow_repetition) {
    int count = 0;
    out_code[0] = '\0';

    for (const char *p = line; p && *p; ++p) {
        char c = *p;
        if (isalpha((unsigned char)c)) {
            c = (char)toupper((unsigned char)c);
            if (!is_valid_color_char(c, color_count))
                return false;

            if (count < CODE_LEN) {
                out_code[count++] = c;
            } else {
                return false; // trop de lettres
            }
        }
    }

    if (count != CODE_LEN) return false;
    out_code[CODE_LEN] = '\0';

    if (!allow_repetition && !has_no_repetition(out_code, CODE_LEN))
        return false;

    return true;
}

/* -------------------------
   Génération du secret + feedback
   ------------------------- */

static void generate_secret(char secret[CODE_LEN + 1], int color_count, bool allow_repetition) {
    if (color_count < MIN_COLORS) color_count = MIN_COLORS;
    if (color_count > MAX_COLORS) color_count = MAX_COLORS;

    if (allow_repetition) {
        for (int i = 0; i < CODE_LEN; i++)
            secret[i] = GLOBAL_COLOR_SET[rand() % color_count];
    } else {
        char pool[MAX_COLORS];
        for (int i = 0; i < color_count; i++) pool[i] = GLOBAL_COLOR_SET[i];

        for (int i = color_count - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            char t = pool[i]; pool[i] = pool[j]; pool[j] = t;
        }

        for (int k = 0; k < CODE_LEN; k++)
            secret[k] = pool[k];
    }

    secret[CODE_LEN] = '\0';
}

static void compute_feedback(const char secret[CODE_LEN + 1], const char guess[CODE_LEN + 1],
                             int *black, int *white) {
    *black = 0;
    *white = 0;

    bool s_used[CODE_LEN] = {0};
    bool g_used[CODE_LEN] = {0};

    for (int i = 0; i < CODE_LEN; i++) {
        if (secret[i] == guess[i]) {
            (*black)++;
            s_used[i] = true;
            g_used[i] = true;
        }
    }

    int sc[256] = {0};
    int gc[256] = {0};

    for (int i = 0; i < CODE_LEN; i++) {
        if (!s_used[i]) sc[(unsigned char)secret[i]]++;
        if (!g_used[i]) gc[(unsigned char)guess[i]]++;
    }

    int matches = 0;
    for (int c = 0; c < 256; c++) {
        if (sc[c] > 0 && gc[c] > 0)
            matches += (sc[c] < gc[c] ? sc[c] : gc[c]);
    }

    *white = matches;
}

/* -------------------------
   Mode chrono strict
   ------------------------- */

static bool timed_get_guess(char out_code[CODE_LEN + 1], int color_count, bool allow_repetition,
                            int time_limit_sec) {
    time_t start = time(NULL);

    char line[256];
    printf("Proposition (ex: RGBY) ou commande (save/quit): ");
    fflush(stdout);

    if (!read_line(line, sizeof(line))) return false;

    time_t end = time(NULL);
    double elapsed = difftime(end, start);

    if (elapsed > (double)time_limit_sec) {
        printf("Temps depasse (%.0fs > %ds). Entree ignoree.\n", elapsed, time_limit_sec);
        return false;
    }

    trim_spaces(line);

    // Ici on laisse la détection de commandes au niveau appelant si besoin,
    // donc on ne traite que la proposition.
    return parse_guess(line, out_code, color_count, allow_repetition);
}

/* -------------------------
   Sauvegarde / chargement
   ------------------------- */

static bool save_game(const GameState *gs, const char *path) {
    if (!gs || !path) return false;
    FILE *f = fopen(path, "w");
    if (!f) return false;

    fprintf(f, "color_count=%d\n", gs->cfg.color_count);
    fprintf(f, "max_tries=%d\n", gs->cfg.max_tries);
    fprintf(f, "allow_repetition=%d\n", gs->cfg.allow_repetition ? 1 : 0);
    fprintf(f, "timed_mode=%d\n", gs->cfg.timed_mode ? 1 : 0);
    fprintf(f, "time_per_try_sec=%d\n", gs->cfg.time_per_try_sec);
    fprintf(f, "tries=%d\n", gs->tries);

    fprintf(f, "secret=%c%c%c%c\n", gs->secret[0], gs->secret[1], gs->secret[2], gs->secret[3]);

    for (int i = 0; i < gs->tries && i < MAX_HISTORY; i++) {
        fprintf(f, "guess%d=%c%c%c%c black=%d white=%d\n",
                i+1,
                gs->guesses[i][0], gs->guesses[i][1], gs->guesses[i][2], gs->guesses[i][3],
                gs->blacks[i], gs->whites[i]);
    }

    fclose(f);
    return true;
}

static bool sanitize_loaded_config(GameConfig *cfg) {
    if (!cfg) return false;

    if (cfg->color_count < MIN_COLORS || cfg->color_count > MAX_COLORS) return false;
    if (cfg->max_tries < MAX_TRIES_MIN || cfg->max_tries > MAX_TRIES_MAX) return false;
    if (cfg->timed_mode) {
        if (cfg->time_per_try_sec < 1 || cfg->time_per_try_sec > 3600) return false;
    } else {
        cfg->time_per_try_sec = 0;
    }
    return true;
}

static bool load_game(GameState *gs, const char *path) {
    if (!gs || !path) return false;

    FILE *f = fopen(path, "r");
    if (!f) return false;

    memset(gs, 0, sizeof(*gs));
    gs->in_progress = true;

    char line[256];
    bool got_cfg_color = false, got_cfg_tries = false, got_secret = false, got_tries = false;

    while (fgets(line, sizeof(line), f)) {
        // config
        if (sscanf(line, "color_count=%d", &gs->cfg.color_count) == 1) { got_cfg_color = true; continue; }
        if (sscanf(line, "max_tries=%d", &gs->cfg.max_tries) == 1) { got_cfg_tries = true; continue; }

        int b;
        if (sscanf(line, "allow_repetition=%d", &b) == 1) { gs->cfg.allow_repetition = (b != 0); continue; }
        if (sscanf(line, "timed_mode=%d", &b) == 1) { gs->cfg.timed_mode = (b != 0); continue; }

        if (sscanf(line, "time_per_try_sec=%d", &gs->cfg.time_per_try_sec) == 1) continue;

        if (sscanf(line, "tries=%d", &gs->tries) == 1) {
            got_tries = true;
            continue;
        }

        if (sscanf(line, "secret=%c%c%c%c",
                   &gs->secret[0], &gs->secret[1], &gs->secret[2], &gs->secret[3]) == 4) {
            gs->secret[CODE_LEN] = '\0';
            got_secret = true;
            continue;
        }

        // historique
        int idx, black, white;
        char g0,g1,g2,g3;

        if (sscanf(line, "guess%d=%c%c%c%c black=%d white=%d",
                   &idx, &g0, &g1, &g2, &g3, &black, &white) == 7) {
            if (idx < 1 || idx > MAX_HISTORY) continue;
            int i = idx - 1;

            // sécurité: bornes black/white
            if (black < 0 || black > CODE_LEN) continue;
            if (white < 0 || white > CODE_LEN) continue;

            gs->guesses[i][0] = (char)toupper((unsigned char)g0);
            gs->guesses[i][1] = (char)toupper((unsigned char)g1);
            gs->guesses[i][2] = (char)toupper((unsigned char)g2);
            gs->guesses[i][3] = (char)toupper((unsigned char)g3);
            gs->guesses[i][CODE_LEN] = '\0';

            gs->blacks[i] = black;
            gs->whites[i] = white;
        }
    }

    fclose(f);

    // validations minimales
    if (!(got_cfg_color && got_cfg_tries && got_secret && got_tries)) return false;
    if (!sanitize_loaded_config(&gs->cfg)) return false;
    if (gs->tries < 0) return false;
    if (gs->tries > MAX_HISTORY) gs->tries = MAX_HISTORY; // on tronque proprement
    // secret doit être composé de lettres valides selon config
    for (int i = 0; i < CODE_LEN; i++) {
        if (!is_valid_color_char(gs->secret[i], gs->cfg.color_count)) return false;
    }
    if (!gs->cfg.allow_repetition && !has_no_repetition(gs->secret, CODE_LEN)) return false;

    return true;
}

/* -------------------------
   Stats (persistantes)
   ------------------------- */

static bool load_stats(Stats *st, const char *path) {
    if (!st || !path) return false;

    FILE *f = fopen(path, "r");
    if (!f) {
        st->games_played = 0;
        st->games_won = 0;
        st->total_tries = 0;
        st->total_time = 0.0;
        return true;
    }

    if (fscanf(f, "%lu %lu %lu %lf",
               &st->games_played, &st->games_won, &st->total_tries, &st->total_time) != 4) {
        st->games_played = 0;
        st->games_won = 0;
        st->total_tries = 0;
        st->total_time = 0.0;
    }

    fclose(f);
    return true;
}

static bool save_stats(const Stats *st, const char *path) {
    if (!st || !path) return false;

    FILE *f = fopen(path, "w");
    if (!f) return false;

    fprintf(f, "%lu %lu %lu %.6f\n",
            st->games_played, st->games_won, st->total_tries, st->total_time);

    fclose(f);
    return true;
}

static void print_stats(const Stats *st) {
    printf("\n=== Statistiques ===\n");
    printf("- Parties jouees: %lu\n", st->games_played);
    printf("- Victoires     : %lu\n", st->games_won);

    double win_rate = (st->games_played > 0)
        ? (100.0 * (double)st->games_won / (double)st->games_played)
        : 0.0;
    printf("- Taux de victoire: %.1f%%\n", win_rate);

    double avg_tries = (st->games_played > 0)
        ? ((double)st->total_tries / (double)st->games_played)
        : 0.0;
    printf("- Tentatives moyennes: %.2f\n", avg_tries);

    double avg_time = (st->games_played > 0)
        ? (st->total_time / (double)st->games_played)
        : 0.0;
    printf("- Temps moyen par partie: %.2fs\n\n", avg_time);
}

/* -------------------------
   IA (heuristique minimax “light”)
   ------------------------- */

static void feedback_between(const char secret[CODE_LEN + 1],
                             const char guess[CODE_LEN + 1],
                             int *black, int *white) {
    compute_feedback(secret, guess, black, white);
}

static int generate_all_codes(char codes[][CODE_LEN + 1], const GameConfig *cfg) {
    int count = 0;

    if (cfg->allow_repetition) {
        for (int i0 = 0; i0 < cfg->color_count; i0++)
        for (int i1 = 0; i1 < cfg->color_count; i1++)
        for (int i2 = 0; i2 < cfg->color_count; i2++)
        for (int i3 = 0; i3 < cfg->color_count; i3++) {
            codes[count][0] = GLOBAL_COLOR_SET[i0];
            codes[count][1] = GLOBAL_COLOR_SET[i1];
            codes[count][2] = GLOBAL_COLOR_SET[i2];
            codes[count][3] = GLOBAL_COLOR_SET[i3];
            codes[count][CODE_LEN] = '\0';
            count++;
        }
    } else {
        for (int i0 = 0; i0 < cfg->color_count; i0++)
        for (int i1 = 0; i1 < cfg->color_count; i1++) if (i1 != i0)
        for (int i2 = 0; i2 < cfg->color_count; i2++) if (i2 != i0 && i2 != i1)
        for (int i3 = 0; i3 < cfg->color_count; i3++) if (i3 != i0 && i3 != i1 && i3 != i2) {
            codes[count][0] = GLOBAL_COLOR_SET[i0];
            codes[count][1] = GLOBAL_COLOR_SET[i1];
            codes[count][2] = GLOBAL_COLOR_SET[i2];
            codes[count][3] = GLOBAL_COLOR_SET[i3];
            codes[count][CODE_LEN] = '\0';
            count++;
        }
    }

    return count;
}

static int evaluate_guess(const char guess[CODE_LEN + 1],
                          char possibles[][CODE_LEN + 1],
                          const bool actif[],
                          int nb_possibles) {
    int worst_partition = 0;
    int counts[25] = {0}; // 0..4 blacks, 0..4 whites

    for (int i = 0; i < nb_possibles; i++) {
        if (!actif[i]) continue;

        int b, w;
        feedback_between(possibles[i], guess, &b, &w);

        int idx = b * 5 + w;
        if (idx >= 0 && idx < 25) counts[idx]++;
    }

    for (int k = 0; k < 25; k++)
        if (counts[k] > worst_partition)
            worst_partition = counts[k];

    return worst_partition;
}

static void choose_next_guess(char guess_out[CODE_LEN + 1],
                              char possibles[][CODE_LEN + 1],
                              bool actif[],
                              int nb_possibles) {
    int best_score = 999999;
    int best_index = -1;

    for (int i = 0; i < nb_possibles; i++) {
        if (!actif[i]) continue;

        int score = evaluate_guess(possibles[i], possibles, actif, nb_possibles);
        if (score < best_score) {
            best_score = score;
            best_index = i;
        }
    }

    if (best_index == -1) {
        for (int i = 0; i < nb_possibles; i++) {
            if (actif[i]) { best_index = i; break; }
        }
    }

    if (best_index != -1) {
        memcpy(guess_out, possibles[best_index], CODE_LEN + 1);
    } else {
        for (int i = 0; i < CODE_LEN; i++) guess_out[i] = GLOBAL_COLOR_SET[0];
        guess_out[CODE_LEN] = '\0';
    }
}

static int filter_possibilities(char possibles[][CODE_LEN + 1],
                                bool actif[],
                                int nb_possibles,
                                const char guess[CODE_LEN + 1],
                                int noirs_attendus,
                                int blancs_attendus) {
    int count = 0;

    for (int i = 0; i < nb_possibles; i++) {
        if (!actif[i]) continue;

        int b, w;
        feedback_between(possibles[i], guess, &b, &w);

        if (b == noirs_attendus && w == blancs_attendus) {
            count++;
        } else {
            actif[i] = false;
        }
    }

    return count;
}

/* -------------------------
   Affichage aide / état
   ------------------------- */

static void print_palette(int color_count) {
    printf("Palette (%d couleurs): ", color_count);
    for (int i = 0; i < color_count; i++) {
        printf("%c=%s", GLOBAL_COLOR_SET[i], GLOBAL_COLOR_NAMES[i]);
        if (i + 1 < color_count) printf(", ");
    }
    printf("\n");
}

static void print_history(const GameState *gs) {
    printf("\n--- Historique ---\n");
    if (!gs || gs->tries <= 0) {
        printf("(aucune tentative)\n");
        return;
    }
    for (int i = 0; i < gs->tries && i < MAX_HISTORY; i++) {
        if (gs->guesses[i][0] == '\0') continue;
        printf("%2d) %s  -> noirs=%d blancs=%d\n", i+1, gs->guesses[i], gs->blacks[i], gs->whites[i]);
    }
}

/* -------------------------
   Boucles de jeu (Humain / IA)
   ------------------------- */

static bool get_guess_or_command(GameState *gs, char out_guess[CODE_LEN + 1], bool *want_quit) {
    *want_quit = false;

    char line[256];
    printf("Proposition (ex: RGBY) ou commande (save/quit/help): ");
    fflush(stdout);

    if (!read_line(line, sizeof(line))) return false;
    trim_spaces(line);

    if (line[0] == '\0') return false;

    if (equals_ignore_case(line, "help")) {
        print_rules();
        return false;
    }

    if (equals_ignore_case(line, "save")) {
        if (gs && gs->in_progress) {
            if (save_game(gs, "save.txt")) printf("Sauvegarde OK (save.txt)\n");
            else printf("Echec sauvegarde.\n");
        } else {
            printf("Aucune partie en cours a sauvegarder.\n");
        }
        return false;
    }

    if (equals_ignore_case(line, "quit")) {
        *want_quit = true;
        return false;
    }

    return parse_guess(line, out_guess, gs->cfg.color_count, gs->cfg.allow_repetition);
}

static void play_human(GameConfig cfg, Stats *st) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    gs.cfg = cfg;
    gs.in_progress = true;

    generate_secret(gs.secret, cfg.color_count, cfg.allow_repetition);

    time_t start_part = time(NULL);

    printf("\n=== Partie HUMAIN ===\n");
    print_palette(cfg.color_count);
    printf("Code: %d positions | Repetitions: %s | Max tentatives: %d\n",
           CODE_LEN, cfg.allow_repetition ? "ON" : "OFF", cfg.max_tries);
    if (cfg.timed_mode) printf("Chrono strict: %ds par tentative (non interruptible, mais entree ignoree si depassee)\n", cfg.time_per_try_sec);
    printf("Tape 'help' pour les regles, 'save' pour sauvegarder, 'quit' pour quitter.\n\n");

    while (gs.tries < cfg.max_tries && gs.tries < MAX_HISTORY) {
        char guess[CODE_LEN + 1] = {0};
        bool ok = false;
        bool want_quit = false;

        printf("\nTentative %d/%d\n", gs.tries + 1, cfg.max_tries);

        if (cfg.timed_mode) {
            // En timed_mode, on ne peut pas intercepter "save/quit" pendant fgets de façon portable.
            // On laisse donc l'utilisateur saisir le guess. Les commandes restent possibles hors chrono via menu.
            ok = timed_get_guess(guess, cfg.color_count, cfg.allow_repetition, cfg.time_per_try_sec);
            if (!ok) {
                printf("Entree invalide / hors temps.\n");
                continue;
            }
        } else {
            ok = get_guess_or_command(&gs, guess, &want_quit);
            if (want_quit) break;
            if (!ok) {
                printf("Entree invalide. (Astuce: ex 'RGBY' ou 'R G B Y')\n");
                continue;
            }
        }

        int black = 0, white = 0;
        compute_feedback(gs.secret, guess, &black, &white);

        memcpy(gs.guesses[gs.tries], guess, CODE_LEN + 1);
        gs.blacks[gs.tries] = black;
        gs.whites[gs.tries] = white;
        gs.tries++;

        printf("-> %s | noirs=%d blancs=%d\n", guess, black, white);
        print_history(&gs);

        if (black == CODE_LEN) {
            time_t end_part = time(NULL);
            double elapsed = difftime(end_part, start_part);

            printf("\n✅ Gagne en %d tentative(s) ! Temps: %.0fs\n", gs.tries, elapsed);

            st->games_played++;
            st->games_won++;
            st->total_tries += (unsigned long)gs.tries;
            st->total_time += elapsed;

            save_stats(st, "stats.txt");
            gs.in_progress = false;

            // optionnel: supprimer save.txt si existait (on n'est pas obligé)
            return;
        }
    }

    // Défaite / quit
    time_t end_part = time(NULL);
    double elapsed = difftime(end_part, start_part);

    if (gs.tries >= cfg.max_tries) {
        printf("\n❌ Perdu. Le secret etait: %s\n", gs.secret);
    } else {
        printf("\nPartie quittee. Le secret etait: %s\n", gs.secret);
    }

    st->games_played++;
    st->total_tries += (unsigned long)gs.tries;
    st->total_time += elapsed;
    save_stats(st, "stats.txt");
}

static void play_ai(GameConfig cfg, Stats *st) {
    char secret[CODE_LEN + 1];
    generate_secret_ai(secret, cfg.color_count, cfg.allow_repetition);

    // taille max: 6^4 = 1296 (répétition ON). Sans répétition: 6P4 = 360.
    char possibles[2000][CODE_LEN + 1];
    bool actif[2000];

    int nb_possibles = generate_all_codes(possibles, &cfg);
    for (int i = 0; i < nb_possibles; i++) actif[i] = true;

    int tries = 0;
    time_t start_part = time(NULL);

    printf("\n=== Partie IA ===\n");
    print_palette(cfg.color_count);
    printf("Secret (cache): %s\n", secret);
    printf("L'IA va tenter de trouver le secret en %d essais max.\n\n", cfg.max_tries);

    while (tries < cfg.max_tries) {
        char guess[CODE_LEN + 1];
        choose_next_guess(guess, possibles, actif, nb_possibles);

        int black = 0, white = 0;
        compute_feedback(secret, guess, &black, &white);
        tries++;

        printf("IA tentative %d: %s -> noirs=%d blancs=%d\n", tries, guess, black, white);

        if (black == CODE_LEN) {
            time_t end_part = time(NULL);
            double elapsed = difftime(end_part, start_part);

            printf("✅ IA a trouve en %d tentative(s). Temps: %.0fs\n", tries, elapsed);

            st->games_played++;
            st->total_tries += (unsigned long)tries;
            st->total_time += elapsed;
            save_stats(st, "stats.txt");
            return;
        }

        filter_possibilities(possibles, actif, nb_possibles, guess, black, white);
    }

    time_t end_part = time(NULL);
    double elapsed = difftime(end_part, start_part);

    printf("❌ IA n'a pas trouve. Secret: %s | Temps: %.0fs\n", secret, elapsed);

    st->games_played++;
    st->total_tries += (unsigned long)tries;
    st->total_time += elapsed;
    save_stats(st, "stats.txt");
}

/* -------------------------
   Menu + config utilisateur
   ------------------------- */

static int ask_int(const char *prompt, int minv, int maxv) {
    char buf[128];
    int val;

    while (1) {
        printf("%s [%d..%d]: ", prompt, minv, maxv);
        fflush(stdout);
        if (!read_line(buf, sizeof(buf))) continue;
        trim_spaces(buf);
        if (sscanf(buf, "%d", &val) == 1 && val >= minv && val <= maxv) return val;
        printf("Valeur invalide.\n");
    }
}

static bool ask_yes_no(const char *prompt) {
    char buf[64];

    while (1) {
        printf("%s (o/n): ", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof(buf))) continue;
        trim_spaces(buf);

        if (buf[0]=='o'||buf[0]=='O'||buf[0]=='y'||buf[0]=='Y') return true;
        if (buf[0]=='n'||buf[0]=='N') return false;

        printf("Reponse invalide.\n");
    }
}

static void configure_game(GameConfig *cfg) {
    printf("\n=== Configuration ===\n");
    printf("1) Facile\n");
    printf("2) Intermediaire\n");
    printf("3) Difficile\n");
    printf("4) Expert\n");
    printf("5) Personnalise\n");
    printf("0) Retour\n");
    printf("Choix: ");
    fflush(stdout);

    char line[64];
    if (!read_line(line, sizeof(line))) return;
    trim_spaces(line);

    int c = atoi(line);

    switch (c) {
        case 1: set_preset_easy(cfg); break;
        case 2: set_preset_intermediate(cfg); break;
        case 3: set_preset_hard(cfg); break;
        case 4: set_preset_expert(cfg); break;
        case 5:
            cfg->color_count = ask_int("Nombre de couleurs", MIN_COLORS, MAX_COLORS);
            cfg->max_tries   = ask_int("Nombre de tentatives", MAX_TRIES_MIN, MAX_TRIES_MAX);
            cfg->allow_repetition = ask_yes_no("Autoriser les repetitions ?");
            cfg->timed_mode = ask_yes_no("Activer le chronometre strict ?");
            cfg->time_per_try_sec = cfg->timed_mode ? ask_int("Temps par tentative (s)", 10, 300) : 0;
            break;
        default:
            break;
    }

    printf("\nConfig actuelle: couleurs=%d, essais=%d, repetitions=%s, chrono=%s",
           cfg->color_count, cfg->max_tries,
           cfg->allow_repetition ? "ON" : "OFF",
           cfg->timed_mode ? "ON" : "OFF");
    if (cfg->timed_mode) printf(" (%ds)", cfg->time_per_try_sec);
    printf("\n\n");
}

static void print_menu(const GameConfig *cfg) {
    printf("=====================================\n");
    printf(" Mastermind (console)\n");
    printf("-------------------------------------\n");
    printf("Config: couleurs=%d, essais=%d, rep=%s, chrono=%s",
           cfg->color_count, cfg->max_tries,
           cfg->allow_repetition ? "ON" : "OFF",
           cfg->timed_mode ? "ON" : "OFF");
    if (cfg->timed_mode) printf("(%ds)", cfg->time_per_try_sec);
    printf("\n");
    printf("-------------------------------------\n");
    printf("1) Jouer (humain)\n");
    printf("2) Lancer IA\n");
    printf("3) Configurer\n");
    printf("4) Regles\n");
    printf("5) Statistiques\n");
    printf("6) Reprendre (save.txt)\n");
    printf("0) Quitter\n");
    printf("Choix: ");
    fflush(stdout);
}

static void menu_loop(void) {
    GameConfig cfg;
    set_default_config(&cfg);

    Stats stats;
    load_stats(&stats, "stats.txt");

    // seed RNG une seule fois
    srand((unsigned int)time(NULL));

    while (1) {
        print_menu(&cfg);

        char line[64];
        if (!read_line(line, sizeof(line))) continue;
        trim_spaces(line);

        int choice = atoi(line);

        switch (choice) {
            case 1: play_human(cfg, &stats); break;
            case 2: play_ai(cfg, &stats); break;
            case 3: configure_game(&cfg); break;
            case 4: print_rules(); break;
            case 5: print_stats(&stats); break;
            case 6: resume_game(&stats); break;
            case 0: printf("Bye.\n"); return;
            default: printf("Choix invalide.\n"); break;
        }
    }
}

/* -------------------------
   Fonctions manquantes (corrigées)
   ------------------------- */

void generate_secret_ai(char secret[CODE_LEN + 1], int color_count, bool allow_repetition) {
    // Pour l'instant, l'IA “crée” un secret aléatoire identique au mode humain.
    // Garder cette fonction séparée te permet de changer la logique plus tard si tu veux.
    generate_secret(secret, color_count, allow_repetition);
}

void print_rules(void) {
    printf("\n=== Regles Mastermind ===\n");
    printf("- Le code secret contient %d lettres (couleurs).\n", CODE_LEN);
    printf("- Vous proposez une combinaison de %d lettres parmi la palette.\n", CODE_LEN);
    printf("- Feedback:\n");
    printf("  * noir  = bonne couleur a la bonne position\n");
    printf("  * blanc = bonne couleur mais mauvaise position\n");
    printf("- Commandes pendant une partie (mode non-chrono):\n");
    printf("  * save : sauvegarde dans save.txt\n");
    printf("  * quit : quitte la partie\n");
    printf("  * help : affiche ces regles\n");
    printf("- Exemple de saisie: RGBY, ou R G B Y\n\n");
}

void resume_game(Stats *st) {
    GameState gs;
    if (!load_game(&gs, "save.txt")) {
        printf("\nImpossible de charger save.txt (absent ou corrompu).\n\n");
        return;
    }

    printf("\n=== Reprise de partie (save.txt) ===\n");
    print_palette(gs.cfg.color_count);
    printf("Config: essais max=%d, repetitions=%s, chrono=%s",
           gs.cfg.max_tries,
           gs.cfg.allow_repetition ? "ON" : "OFF",
           gs.cfg.timed_mode ? "ON" : "OFF");
    if (gs.cfg.timed_mode) printf("(%ds)", gs.cfg.time_per_try_sec);
    printf("\n");
    printf("Tentatives deja faites: %d\n", gs.tries);
    print_history(&gs);

    time_t start_part = time(NULL);

    // Rejouer à partir de l'état chargé
    while (gs.tries < gs.cfg.max_tries && gs.tries < MAX_HISTORY) {
        char guess[CODE_LEN + 1] = {0};
        bool ok = false;
        bool want_quit = false;

        printf("\nTentative %d/%d\n", gs.tries + 1, gs.cfg.max_tries);

        if (gs.cfg.timed_mode) {
            ok = timed_get_guess(guess, gs.cfg.color_count, gs.cfg.allow_repetition, gs.cfg.time_per_try_sec);
            if (!ok) {
                printf("Entree invalide / hors temps.\n");
                continue;
            }
        } else {
            ok = get_guess_or_command(&gs, guess, &want_quit);
            if (want_quit) break;
            if (!ok) {
                printf("Entree invalide.\n");
                continue;
            }
        }

        int black = 0, white = 0;
        compute_feedback(gs.secret, guess, &black, &white);

        memcpy(gs.guesses[gs.tries], guess, CODE_LEN + 1);
        gs.blacks[gs.tries] = black;
        gs.whites[gs.tries] = white;
        gs.tries++;

        printf("-> %s | noirs=%d blancs=%d\n", guess, black, white);
        print_history(&gs);

        if (black == CODE_LEN) {
            time_t end_part = time(NULL);
            double elapsed = difftime(end_part, start_part);

            printf("\n✅ Gagne (reprise) en %d tentative(s) ! Temps (reprise): %.0fs\n", gs.tries, elapsed);

            st->games_played++;
            st->games_won++;
            st->total_tries += (unsigned long)gs.tries;
            st->total_time += elapsed;
            save_stats(st, "stats.txt");

            gs.in_progress = false;
            // Tu peux choisir de supprimer save.txt ici si tu veux.
            return;
        }
    }

    time_t end_part = time(NULL);
    double elapsed = difftime(end_part, start_part);

    if (gs.tries >= gs.cfg.max_tries) {
        printf("\n❌ Perdu (reprise). Le secret etait: %s\n", gs.secret);
    } else {
        printf("\nPartie reprise quittee. Le secret etait: %s\n", gs.secret);
        // On laisse la sauvegarde intacte, mais tu peux aussi la mettre à jour
        // save_game(&gs, "save.txt");
    }

    st->games_played++;
    st->total_tries += (unsigned long)gs.tries;
    st->total_time += elapsed;
    save_stats(st, "stats.txt");
}

/* -------------------------
   main
   ------------------------- */

int main(void) {
    menu_loop();
    return 0;
}
