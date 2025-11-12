#include "colors.h"
#include <stdio.h>
#include <ctype.h>

const char COLOR_SET[COLOR_COUNT] = { 'R', 'G', 'B', 'Y', 'O', 'P' };
const char *COLOR_NAMES[COLOR_COUNT] = { "Rouge", "Vert", "Bleu", "Jaune", "Orange", "Violet" };

void print_palette(void) {
    printf("Couleurs disponibles (sans repetition):\n");
    for (int i = 0; i < COLOR_COUNT; i++) {
        printf("  %c = %s\n", COLOR_SET[i], COLOR_NAMES[i]);
    }
    printf("Saisissez 4 lettres (ex: RGBY ou R G B Y).\n");
}

bool is_valid_color_char(char c) {
    c = (char)toupper((unsigned char)c);
    for (int i = 0; i < COLOR_COUNT; i++) {
        if (COLOR_SET[i] == c) return true;
    }
    return false;
}

void print_code(const char code[CODE_LEN]) {
    for (int i = 0; i < CODE_LEN; i++) {
        printf("%c", code[i]);
    }
}
