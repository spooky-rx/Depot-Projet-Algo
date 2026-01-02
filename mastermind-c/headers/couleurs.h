#ifndef COULEURS_H
#define COULEURS_H

#include "types.h"

extern const char GLOBAL_COLOR_SET[MAX_COLORS];
extern const char *GLOBAL_COLOR_NAMES[MAX_COLORS];

void afficher_palette(int color_count);
void afficher_code(const char code[CODE_LEN]);

#endif
