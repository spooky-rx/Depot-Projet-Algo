#ifndef CHRONOMETRE_H
#define CHRONOMETRE_H

#include <stdbool.h>
#include "types.h"

bool saisie_minutee(char out_code[CODE_LEN],
                    int color_count, bool allow_repetition,
                    int time_limit_sec);

#endif
