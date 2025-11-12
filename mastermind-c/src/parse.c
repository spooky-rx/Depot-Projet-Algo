#include "parse.h"
#include "utils.h"
#include <ctype.h>
#include <string.h>

/**
 * Parcourt la ligne, extrait les lettres alphabetiques,
 * valide contre la palette, normalise en majuscules.
 */
bool parse_guess(const char *line, char out_code[CODE_LEN]) {
    int count = 0;
    for (const char *p = line; *p != '\0'; ++p) {
        char c = *p;
        if (isalpha((unsigned char)c)) {
            c = (char)toupper((unsigned char)c);
            if (!is_valid_color_char(c)) {
                return false; // lettre alphabetique mais pas couleur autorisee
            }
            if (count < CODE_LEN) {
                out_code[count++] = c;
            } else {
                return false; // trop de lettres valides
            }
        }
        // ignore espaces, virgules, autres separateurs
    }
    if (count != CODE_LEN) return false;
    if (!has_no_repetition(out_code, CODE_LEN)) return false;
    return true;
}
