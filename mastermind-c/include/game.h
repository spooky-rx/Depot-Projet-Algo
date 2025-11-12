#ifndef GAME_H
#define GAME_H

#include "colors.h"

/**
 * Genere un code secret de 4 couleurs sans repetition.
 */
void generate_secret(char secret[CODE_LEN]);

/**
 * Lance la boucle principale du jeu (I/O console).
 */
void run_game(void);

#endif // GAME_H
