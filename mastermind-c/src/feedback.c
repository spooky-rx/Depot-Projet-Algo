#include "feedback.h"
#include <stdbool.h>

void compute_feedback(const char secret[CODE_LEN],
                      const char guess[CODE_LEN],
                      int *black, int *white) {
    *black = 0;
    *white = 0;

    bool secret_used[CODE_LEN] = { false };
    bool guess_used[CODE_LEN]  = { false };

    // 1) noirs (positions exactes)
    for (int i = 0; i < CODE_LEN; i++) {
        if (secret[i] == guess[i]) {
            (*black)++;
            secret_used[i] = true;
            guess_used[i]  = true;
        }
    }

    // 2) blancs (comptage des occurrences restantes)
    int secret_count[256] = {0};
    int guess_count[256]  = {0};

    for (int i = 0; i < CODE_LEN; i++) {
        if (!secret_used[i]) secret_count[(unsigned char)secret[i]]++;
        if (!guess_used[i])  guess_count[(unsigned char)guess[i]]++;
    }

    int matches = 0;
    for (int c = 0; c < 256; c++) {
        int s = secret_count[c], g = guess_count[c];
        if (s > 0 && g > 0) {
            matches += (s < g) ? s : g;
        }
    }

    *white = matches;
}
