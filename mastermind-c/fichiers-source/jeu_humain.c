#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "jeu_humain.h"
#include "couleurs.h"
#include "parse.h"
#include "feedback.h"
#include "chronometre.h"
#include "sauvegarde.h"
#include "statistiques.h"
#include "utils.h"

static void generer_secret(char secret[CODE_LEN],
                           int color_count, bool allow_repetition) {
    if (allow_repetition) {
        for (int i=0;i<CODE_LEN;i++)
            secret[i] = GLOBAL_COLOR_SET[rand()%color_count];
    } else {
        char pool[MAX_COLORS];
        for (int i=0;i<color_count;i++) pool[i]=GLOBAL_COLOR_SET[i];
        for (int i=color_count-1;i>0;i--) {
            int j=rand()%(i+1);
            char t=pool[i]; pool[i]=pool[j]; pool[j]=t;
        }
        for (int k=0;k<CODE_LEN;k++) secret[k]=pool[k];
    }
}

static void afficher_historique(const GameState *gs) {
    printf("Historique des essais:\n");
    for (int i=0;i<gs->tries;i++) {
        printf("  %2d) ", i+1);
        afficher_code(gs->guesses[i]);
        printf("  => noirs: %d, blancs: %d\n",
               gs->blacks[i], gs->whites[i]);
    }
}

static void bannière(void) {
    printf("\n=====================================\n");
    printf("        Mastermind - Avance          \n");
    printf("=====================================\n\n");
}

void jouer_humain(GameConfig cfg, Stats *st) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    gs.cfg = cfg;
    gs.in_progress = true;

    srand((unsigned int)time(NULL));

    bannière();
    afficher_palette(cfg.color_count);
    printf("Objectif: devinez le code (%d lettres) en %d tentatives.\n",
           CODE_LEN, cfg.max_tries);
    printf("Options: repetitions %s, chrono %s",
           cfg.allow_repetition?"ON":"OFF",
           cfg.timed_mode?"ON":"OFF");
    if (cfg.timed_mode) printf(" (%ds)", cfg.time_per_try_sec);
    printf("\nFeedback: noirs = bien places, blancs = bonne couleur, mauvaise position.\n\n");

    generer_secret(gs.secret, cfg.color_count, cfg.allow_repetition);

    time_t start_part = time(NULL);

    while (gs.tries < cfg.max_tries) {
        printf("Tentative %d/%d - Votre proposition: ",
               gs.tries+1, cfg.max_tries);

        char guess[CODE_LEN];
        bool ok=false;

        if (cfg.timed_mode) {
            ok = saisie_minutee(guess, cfg.color_count,
                                cfg.allow_repetition, cfg.time_per_try_sec);
        } else {
            char line[256];
            if (!lire_ligne(line, sizeof(line))) {
                printf("Lecture invalide.\n");
                continue;
            }
            ok = parser_proposition(line, guess,
                                    cfg.color_count, cfg.allow_repetition);
        }

        if (!ok) {
            printf("Entree invalide ou hors temps. Rappel: %d lettres parmi ",
                   CODE_LEN);
            for (int i=0;i<cfg.color_count;i++) {
                printf("%c%s", GLOBAL_COLOR_SET[i],
                       (i+1<cfg.color_count)?" ":"");
            }
            printf(", %s repetition.\n\n",
                   cfg.allow_repetition?"avec":"sans");
            printf("Tapez 'save' pour sauvegarder la partie, ou reessayez.\n");
            continue;
        }

        int noirs=0, blancs=0;
        calculer_feedback(gs.secret, guess, &noirs, &blancs);

        memcpy(gs.guesses[gs.tries], guess, CODE_LEN);
        gs.blacks[gs.tries]=noirs;
        gs.whites[gs.tries]=blancs;
        gs.tries++;

        printf("Vous avez propose: ");
        afficher_code(guess);
        printf("  => noirs: %d, blancs: %d\n", noirs, blancs);
        afficher_historique(&gs);
        printf("\n");

        if (noirs == CODE_LEN) {
            time_t end_part = time(NULL);
            double elapsed = difftime(end_part, start_part);
            printf("Bravo ! Code trouve en %d tentative(s).\n", gs.tries);
            printf("Code secret: "); afficher_code(gs.secret); printf("\n");
            st->games_played++;
            st->games_won++;
            st->total_tries += gs.tries;
            st->total_time += elapsed;
            sauvegarder_stats(st, "stats.txt");
            gs.in_progress=false;
            return;
        }

        printf("Commande (enter pour continuer) [save/quit]: ");
        char cmd[32];
        if (lire_ligne(cmd, sizeof(cmd))) {
            if (strcmp(cmd,"save")==0) {
                if (sauvegarder_partie(&gs, "save.txt"))
                    printf("Partie sauvegardee.\n");
                else
                    printf("Echec sauvegarde.\n");
            } else if (strcmp(cmd,"quit")==0) {
                printf("Abandon de la partie.\n");
                break;
            }
        }
    }

    time_t end_part = time(NULL);
    double elapsed = difftime(end_part, start_part);
    printf("Dommage ! Vous n'avez pas trouve le code.\n");
    printf("Le code secret etait: "); afficher_code(gs.secret); printf("\n");
    st->games_played++;
    st->total_tries += gs.tries;
    st->total_time += elapsed;
    sauvegarder_stats(st, "stats.txt");
}
