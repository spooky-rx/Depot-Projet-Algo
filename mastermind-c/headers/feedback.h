#ifndef FEEDBACK_H
#define FEEDBACK_H

#include "types.h"

void calculer_feedback(const char secret[CODE_LEN],
                       const char guess[CODE_LEN],
                       int *noirs, int *blancs);

#endif
