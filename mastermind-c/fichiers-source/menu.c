#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "menu.h"
#include "types.h"
#include "configuration.h"
#include "jeu_humain.h"
#include "ia.h"
#include "statistiques.h"
#include "sauvegarde.h"
#include "couleurs.h"
#include "feedback.h"
#include "parse.h"
#include "utils.h"

static void afficher_regles(void) {
    printf("\n=== Règles & Options ===\n");
    printf("- Code: %d lettres parmi 3..6 couleurs.\n", CODE_LEN);
    printf("- Tentatives: 5..30.\n");
    printf("- Feedback: noirs = bien places, blancs = bonne couleur, mauvaise position.\n");
    printf("- Repetitions: ON/OFF.\n");
    printf("- Chronometre: tentative annulee si temps depasse.\n");
    printf("- Presets: facile, intermediaire, difficile, expert.\n");
    printf("- Modes: Humain vs Code, IA qui devine.\n");
    printf("- Sauvegarde dans save.txt, Statistiques dans stats.txt.\n\n");
}

static void reprendre_partie(Stats *st) {
    GameState gs;
    if (!charger_partie(&gs, "save.txt") || !gs.in_progress) {
        printf("Aucune sauvegarde disponible.\n");
        return;
    }

    printf("\nReprise de partie. Tentatives deja effectuees: %d/%d\n",
           gs.tries, gs.cfg.max_tries);
    afficher_palette(gs.cfg.color_count);

    for (int i=0;i<gs.tries;i++) {
        printf("  %2d) ", i+1);
        afficher_code(gs.guesses[i]);
        printf("  => noirs: %d, blancs: %d\n",
               gs.blacks[i], gs.whites[i]);
    }
    printf("\n");

    time_t start_part = time(NULL);

    while (gs.tries < gs.cfg.max_tries) {
        printf("Tentative %d/%d - Votre proposition: ",
               gs.tries+1, gs.cfg.max_tries);
        char guess[CODE_LEN]; bool ok=false;
        if (gs.cfg.timed_mode) {
            ok = saisie_minutee(guess, gs.cfg.color_count,
                                gs.cfg.allow_repetition, gs.cfg.time_per_try_sec);
        } else {
            char line[256];
            if (!lire_ligne(line, sizeof(line))) {
                printf("Lecture invalide.\n");
                continue;
            }
            ok = parser_proposition(line, guess,
                                    gs.cfg.color_count, gs.cfg.allow_repetition);
        }
        if (!ok) {
            printf("Entree invalide ou hors temps.\n");
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
            FILE *f=fopen("save.txt","w");
            if (f) fclose(f);
            return;
        }

        sauvegarder_partie(&gs, "save.txt");
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

void boucle_menu_avance(void) {
    GameConfig cfg;
    config_defaut(&cfg);

    Stats stats;
    charger_stats(&stats, "stats.txt");

    srand((unsigned int)time(NULL));

    while (1) {
        printf("=== Menu Principal (Avance) ===\n");
        printf("1) Jouer (Humain)\n");
        printf("2) Jouer (IA)\n");
        printf("3) Configurer\n");
        printf("4) Afficher les regles\n");
        printf("5) Afficher les statistiques\n");
        printf("6) Reprendre une partie (charger)\n");
        printf("0) Quitter\n");
        printf("Choix: ");

        char line[64];
        if (!lire_ligne(line, sizeof(line))) continue;
        int choix = atoi(line);

        switch (choix) {
            case 1: jouer_humain(cfg, &stats); break;
            case 2: jouer_ia(cfg, &stats); break;
            case 3: configurer_jeu(&cfg); break;
            case 4: afficher_regles(); break;
            case 5: afficher_stats(&stats); break;
            case 6: reprendre_partie(&stats); break;
            case 0:
                printf("Au revoir !\n");
                return;
            default:
                printf("Choix invalide.\n");
                break;
        }
        printf("\n");
    }
}
