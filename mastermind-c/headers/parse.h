#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>
#include "types.h"

bool caractere_couleur_valide(char c, int color_count);
bool sans_repetition(const char code[], int len);
bool parser_proposition(const char *ligne, char out_code[CODE_LEN],
                        int color_count, bool allow_repetition);

#endif
