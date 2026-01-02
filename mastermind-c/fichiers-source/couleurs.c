#include <stdio.h>
#include "couleurs.h"

const char GLOBAL_COLOR_SET[MAX_COLORS]   = { 'R','G','B','Y','O','P' };
const char *GLOBAL_COLOR_NAMES[MAX_COLORS]= { "Rouge","Vert","Bleu","Jaune","Orange","Violet" };

void afficher_palette(int color_count) {
    printf("Palette:\n");
    for (int i=0;i<color_count;i++) {
        printf("  %c = %s\n", GLOBAL_COLOR_SET[i], GLOBAL_COLOR_NAMES[i]);
    }
}

void afficher_code(const char code[CODE_LEN]) {
    for (int i=0;i<CODE_LEN;i++) {
        printf("%c", code[i]);
    }
}
