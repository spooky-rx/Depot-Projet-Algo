#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Lit une ligne depuis stdin dans buffer (taille buflen).
 * Retire le '\n' final si present. Retourne true si OK.
 */
bool read_line(char *buffer, size_t buflen);

/**
 * Vérifie l'absence de repetitions dans un code de longueur CODE_LEN.
 */
bool has_no_repetition(const char code[], int len);

#endif // UTILS_H
