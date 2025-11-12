#include "game.h"
#include "colors.h"
#include "feedback.h"
#include "parse.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * Generation du secret: melange partiel de la palette (Fisher-Yates),
 * puis selection des 4 premiers caracteres.
 */
void generate_secret(char secret[CODE_LEN]) {
    char pool[COLOR_COUNT];
    for (int i = 0; i < COLOR_COUNT; i++) pool[i] = COLOR_SET[i];

    for (int i = COLOR_COUNT - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        char tmp = pool[i];
        pool[i] = pool[j];
        pool[j] = tmp;
    }

    for (int k = 0; k < CODE_LEN; k++) {
        secret[k] = pool[k];
    }
}

void run_game(void) {
    srand((unsigned int)time(NULL));

    printf("=== Mastermind (Jeu de base) ===\n\n");
    print_palette();
    printf("\nObjectif: devinez le code secret en %d tentatives.\n", MAX_TRIES);
    printf("Feedback: ● = noir (bien place), ○ = blanc (bonne couleur, mauvaise position)\n\n");

    char secret[CODE_LEN];
    generate_secret(secret);

    char history_guess[MAX_TRIES][CODE_LEN];
    int history_black[MAX_TRIES];
    int history_white[MAX_TRIES];
    int tries = 0;

    while (tries < MAX_TRIES) {
        printf("Tentative %d/%d - Entrez votre proposition: ", tries + 1, MAX_TRIES);
        char line[256];
        if (!read_line(line, sizeof(line))) {
            printf("\nErreur de lecture. Veuillez reessayer.\n");
            continue;
        }

        char guess[CODE_LEN];
        if (!parse_guess(line, guess)) {
            printf("Entree invalide. Rappel: 4 lettres parmi R G B Y O P, sans repetition (ex: RGBY).\n");
            continue;
        }

        int black = 0, white = 0;
        compute_feedback(secret, guess, &black, &white);

        for (int i = 0; i < CODE_LEN; i++) history_guess[tries][i] = guess[i];
        history_black[tries] = black;
        history_white[tries] = white;

        printf("Vous avez propose: ");
        print_code(guess);
        printf("  => ●: %d, ○: %d\n", black, white);

        tries++;

        if (black == CODE_LEN) {
            printf("\nBravo ! Vous avez devine le code en %d tentative%s.\n",
                   tries, (tries > 1 ? "s" : ""));
            printf("Code secret: ");
            print_code(secret);
            printf("\n");
            break;
        }

        printf("Historique des essais:\n");
        for (int i = 0; i < tries; i++) {
            printf("  %2d) ", i + 1);
            print_code(history_guess[i]);
            printf("  ●: %d, ○: %d\n", history_black[i], history_white[i]);
        }
        printf("\n");
    }

    if (tries == MAX_TRIES) {
        printf("Dommage ! Vous n'avez pas trouve le code.\n");
        printf("Le code secret etait: ");
        print_code(secret);
        printf("\n");
    }

    printf("\nMerci d'avoir joue !\n");
}
