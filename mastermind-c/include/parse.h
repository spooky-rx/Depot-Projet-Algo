#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>
#include "colors.h"

/**
 * Parse une entree utilisateur en un code de 4 lettres.
 * - Insensible a la casse
 * - Ignore espaces/virgules
 * - Refuse caracteres non valides
 * - Refuse repetitions
 */
bool parse_guess(const char *line, char out_code[CODE_LEN]);

#endif // PARSE_H
