#include <stdbool.h>
#include <string.h>
#include "feedback.h"

void calculer_feedback(const char secret[CODE_LEN],
                       const char guess[CODE_LEN],
                       int *noirs, int *blancs) {
    *noirs = 0;
    *blancs = 0;

    bool s_used[CODE_LEN] = {0};
    bool g_used[CODE_LEN] = {0};

    for (int i=0;i<CODE_LEN;i++) {
        if (secret[i] == guess[i]) {
            (*noirs)++;
            s_used[i] = true;
            g_used[i] = true;
        }
    }

    int sc[256] = {0};
    int gc[256] = {0};

    for (int i=0;i<CODE_LEN;i++) {
        if (!s_used[i]) sc[(unsigned char)secret[i]]++;
        if (!g_used[i]) gc[(unsigned char)guess[i]]++;
    }

    int matches=0;
    for (int c=0;c<256;c++) {
        if (sc[c]>0 && gc[c]>0) {
            matches += (sc[c] < gc[c] ? sc[c] : gc[c]);
        }
    }
    *blancs = matches;
}
