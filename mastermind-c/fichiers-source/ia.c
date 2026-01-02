#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ia.h"
#include "couleurs.h"
#include "feedback.h"
#include "statistiques.h"

static void ia_generer_proposition(char guess[CODE_LEN], const GameConfig *cfg) {
    if (cfg->allow_repetition) {
        for (int i=0;i<CODE_LEN;i++)
            guess[i] = GLOBAL_COLOR_SET[rand()%cfg->color_count];
    } else {
        int used[MAX_COLORS] = {0};
        int k=0;
        while (k<CODE_LEN) {
            int idx = rand()%cfg->color_count;
            if (!used[idx]) {
                used[idx]=1;
                guess[k++] = GLOBAL_COLOR_SET[idx];
            }
        }
    }
}

static void generer_secret(char secret[CODE_LEN],
                           int color_count, bool allow_repetition) {
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

void jouer_ia(GameConfig cfg, Stats *st) {
    printf("\n[Mode IA] L'ordinateur tente de deviner.\n");
    afficher_palette(cfg.color_count);

    char secret[CODE_LEN];
    generer_secret(secret, cfg.color_count, cfg.allow_repetition);

    printf("Secret: **** (masque)\n\n");

    int tries=0;
    time_t start_part = time(NULL);

    while (tries < cfg.max_tries) {
        char guess[CODE_LEN];
        ia_generer_proposition(guess, &cfg);

        int noirs=0, blancs=0;
        calculer_feedback(secret, guess, &noirs, &blancs);
        tries++;

        printf("IA Tentative %d/%d: ", tries, cfg.max_tries);
        afficher_code(guess);
        printf("  => noirs: %d, blancs: %d\n", noirs, blancs);

        if (noirs == CODE_LEN) {
            time_t end_part = time(NULL);
            double elapsed = difftime(end_part, start_part);
            printf("IA a trouve le code en %d tentative(s).\n", tries);
            printf("Code secret: "); afficher_code(secret); printf("\n");
            st->games_played++;
            st->total_tries += tries;
            st->total_time += elapsed;
            sauvegarder_stats(st, "stats.txt");
            return;
        }
    }

    time_t end_part = time(NULL);
    double elapsed = difftime(end_part, start_part);
    printf("IA n'a pas trouve le code.\n");
    printf("Le code secret etait: "); afficher_code(secret); printf("\n");
    st->games_played++;
    st->total_tries += tries;
    st->total_time += elapsed;
    sauvegarder_stats(st, "stats.txt");
}
