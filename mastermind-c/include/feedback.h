#ifndef FEEDBACK_H
#define FEEDBACK_H

#include "colors.h"

/**
 * Calcule le feedback:
 * - black: couleurs bien placees
 * - white: bonnes couleurs mal placees
 */
void compute_feedback(const char secret[CODE_LEN],
                      const char guess[CODE_LEN],
                      int *black, int *white);

#endif // FEEDBACK_H
