#include <time.h>
#include <stdio.h>
#include "chronometre.h"
#include "utils.h"
#include "parse.h"

bool saisie_minutee(char out_code[CODE_LEN],
                    int color_count, bool allow_repetition,
                    int time_limit_sec) {
    time_t start = time(NULL);
    char line[256];
    if (!lire_ligne(line, sizeof(line))) return false;
    time_t end = time(NULL);
    double elapsed = difftime(end, start);
    if (elapsed > (double)time_limit_sec) {
        printf("Temps depasse (%.0fs > %ds). Tentative annulee.\n", elapsed, time_limit_sec);
        return false;
    }
    return parser_proposition(line, out_code, color_count, allow_repetition);
}
