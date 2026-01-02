#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "types.h"
#include "couleurs.h"
#include "parse.h"
#include "feedback.h"
#include "utils.h"

static void generer_secret_base(char secret[CODE_LEN]) {
    char pool[MAX_COLORS];
    int color_count = 6;
    for (int i=0;i<color_count;i++) pool[i]=GLOBAL_COLOR_SET[i];
    for (int i=color_count-1;i>0;i--) {
        int j = rand()%(i+1);
        char t = pool[i]; pool[i]=pool[j]; pool[j]=t;
    }
    for (int k=0;k<CODE_LEN;k++) secret[k]=pool[k];
}

void lancer_jeu_base(void) {
    srand((unsigned int)time(NULL));

    printf("=== Mastermind (Jeu de base) ===\n\n");
    afficher_palette(6);
    printf("\nObjectif: devinez le code secret en 10 tentatives.\n");
    printf("Feedback: noirs = bien places, blancs = bonne couleur, mauvaise position.\n\n");

    char secret[CODE_LEN];
    generer_secret_base(secret);

    char history_guess[10][CODE_LEN];
    int history_black[10];
    int history_white[10];
    int tries = 0;

    while (tries < 10) {
        printf("Tentative %d/10 - Entrez votre proposition: ", tries+1);
        char line[256];
        if (!lire_ligne(line, sizeof(line))) {
            printf("Erreur de lecture.\n");
            continue;
        }
        char guess[CODE_LEN];
        if (!parser_proposition(line, guess, 6, false)) {
            printf("Entree invalide. 4 lettres parmi R G B Y O P, sans repetition.\n");
            continue;
        }

        int noirs=0, blancs=0;
        calculer_feedback(secret, guess, &noirs, &blancs);

        memcpy(history_guess[tries], guess, CODE_LEN);
        history_black[tries]=noirs;
        history_white[tries]=blancs;

        printf("Vous avez propose: ");
        afficher_code(guess);
        printf("  => noirs: %d, blancs: %d\n", noirs, blancs);

        tries++;

        if (noirs == CODE_LEN) {
            printf("\nBravo ! Vous avez devine le code en %d tentative(s).\n", tries);
            printf("Code secret: "); afficher_code(secret); printf("\n");
            break;
        }

        printf("Historique des essais:\n");
        for (int i=0;i<tries;i++) {
            printf("  %2d) ", i+1);
            afficher_code(history_guess[i]);
            printf("  noirs: %d, blancs: %d\n",
                   history_black[i], history_white[i]);
        }
        printf("\n");
    }

    if (tries == 10) {
        printf("Dommage ! Vous n'avez pas trouve le code.\n");
        printf("Le code secret etait: ");
        afficher_code(secret);
        printf("\n");
    }

    printf("\nMerci d'avoir joue !\n");
}
