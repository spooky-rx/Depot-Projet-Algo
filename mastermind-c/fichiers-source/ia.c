#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "ia.h"
#include "types.h"
#include "couleurs.h"
#include "feedback.h"
#include "statistiques.h"

/* ============================================================
   IA avancée (heuristique type Knuth)
   ============================================================ */

// Compare deux codes via compute_feedback
static void feedback_between(const char secret[CODE_LEN],
                             const char guess[CODE_LEN],
                             int *black, int *white)
{
    compute_feedback(secret, guess, black, white);
}

// Génère toutes les combinaisons possibles selon la config
static int generate_all_codes(char codes[][CODE_LEN], const GameConfig *cfg)
{
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
            count++;
        }
    }

    return count;
}

// Évalue un guess : on cherche la pire partition possible
static int evaluate_guess(const char guess[CODE_LEN],
                          char possibles[][CODE_LEN],
                          const bool actif[],
                          int nb_possibles)
{
    int counts[25];
    for (int i = 0; i < 25; i++) counts[i] = 0;

    for (int i = 0; i < nb_possibles; i++) {
        if (!actif[i]) continue;
        int b, w;
        feedback_between(possibles[i], guess, &b, &w);
        counts[b * 5 + w]++;
    }

    int worst = 0;
    for (int i = 0; i < 25; i++)
        if (counts[i] > worst) worst = counts[i];

    return worst;
}

// Choisit la meilleure proposition
static void choose_next_guess(char guess_out[CODE_LEN],
                              char possibles[][CODE_LEN],
                              bool actif[],
                              int nb_possibles)
{
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
        for (int i = 0; i < nb_possibles; i++)
            if (actif[i]) { best_index = i; break; }
    }

    memcpy(guess_out, possibles[best_index], CODE_LEN);
}

// Filtre les possibilités selon le feedback
static int filter_possibilities(char possibles[][CODE_LEN],
                                bool actif[],
                                int nb_possibles,
                                const char guess[CODE_LEN],
                                int black_expected,
                                int white_expected)
{
    int count = 0;

    for (int i = 0; i < nb_possibles; i++) {
        if (!actif[i]) continue;

        int b, w;
        feedback_between(possibles[i], guess, &b, &w);

        if (b == black_expected && w == white_expected)
            count++;
        else
            actif[i] = false;
    }

    return count;
}

// Exemple de possibilité restante
static void print_one_example(char possibles[][CODE_LEN],
                              const bool actif[],
                              int nb_possibles)
{
    for (int i = 0; i < nb_possibles; i++) {
        if (actif[i]) {
            printf("Exemple de code encore possible: ");
            print_code(possibles[i]);
            printf("\n");
            return;
        }
    }
}

// Génère le secret pour l’IA
static void generate_secret_ai(char secret[CODE_LEN],
                               int color_count, bool allow_repetition)
{
    if (allow_repetition) {
        for (int i=0;i<CODE_LEN;i++)
            secret[i] = GLOBAL_COLOR_SET[rand()%color_count];
    } else {
        char pool[MAX_COLORS];
        for (int i=0;i<color_count;i++) pool[i]=GLOBAL_COLOR_SET[i];
        for (int i=color_count-1;i>0;i--) {
            int j = rand()%(i+1);
            char t = pool[i]; pool[i]=pool[j]; pool[j]=t;
        }
        for (int k=0;k<CODE_LEN;k++) secret[k]=pool[k];
    }
}

/* ============================================================
   Fonction principale IA
   ============================================================ */

void play_ai(GameConfig cfg, Stats *st)
{
    printf("\n=== Mode IA (stratégie avancée) ===\n");
    print_palette(cfg.color_count);

    char secret[CODE_LEN];
    generate_secret_ai(secret, cfg.color_count, cfg.allow_repetition);

    printf("Secret: **** (masqué)\n\n");

    char possibles[2000][CODE_LEN];
    bool actif[2000];
    int nb_possibles = generate_all_codes(possibles, &cfg);

    for (int i = 0; i < nb_possibles; i++)
        actif[i] = true;

    printf("Nombre initial de possibilités : %d\n\n", nb_possibles);

    int tries = 0;
    time_t start = time(NULL);

    while (tries < cfg.max_tries) {
        char guess[CODE_LEN];
        choose_next_guess(guess, possibles, actif, nb_possibles);

        int black = 0, white = 0;
        compute_feedback(secret, guess, &black, &white);
        tries++;

        printf("IA Tentative %d/%d : ", tries, cfg.max_tries);
        print_code(guess);
        printf("  => ●: %d, ○: %d\n", black, white);

        if (black == CODE_LEN) {
            time_t end = time(NULL);
            double elapsed = difftime(end, start);

            printf("IA a trouvé le code en %d tentatives.\n", tries);
            printf("Code secret : ");
            print_code(secret);
            printf("\n");

            st->games_played++;
            st->total_tries += tries;
            st->total_time += elapsed;
            save_stats(st, "stats.txt");
            return;
        }

        int before = 0;
        for (int i = 0; i < nb_possibles; i++)
            if (actif[i]) before++;

        int after = filter_possibilities(possibles, actif, nb_possibles,
                                         guess, black, white);

        printf("Raisonnement IA : %d possibilités -> %d après filtrage.\n",
               before, after);

        print_one_example(possibles, actif, nb_possibles);
        printf("\n");
    }

    printf("IA n'a pas trouvé le code.\n");
    printf("Le code secret était : ");
    print_code(secret);
    printf("\n");
}
