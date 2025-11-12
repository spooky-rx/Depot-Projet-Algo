#ifndef COLORS_H
#define COLORS_H

#include <stdbool.h>

#define CODE_LEN 4
#define MAX_TRIES 10
#define COLOR_COUNT 6

extern const char COLOR_SET[COLOR_COUNT];
extern const char *COLOR_NAMES[COLOR_COUNT];

/**
 * Affiche la palette des couleurs disponibles.
 */
void print_palette(void);

/**
 * Vérifie si un caractère correspond à une couleur valide (insensible à la casse).
 */
bool is_valid_color_char(char c);

/**
 * Affiche un code (suite de 4 lettres) sur stdout (sans retour à la ligne).
 */
void print_code(const char code[CODE_LEN]);

#endif // COLORS_H
