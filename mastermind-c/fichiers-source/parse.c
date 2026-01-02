#include <ctype.h>
#include "parse.h"
#include "couleurs.h"

bool caractere_couleur_valide(char c, int color_count) {
    c = (char)toupper((unsigned char)c);
    for (int i=0;i<color_count;i++) {
        if (GLOBAL_COLOR_SET[i]==c) return true;
    }
    return false;
}

bool sans_repetition(const char code[], int len) {
    for (int i=0;i<len;i++) {
        for (int j=i+1;j<len;j++) {
            if (code[i]==code[j]) return false;
        }
    }
    return true;
}

bool parser_proposition(const char *ligne, char out_code[CODE_LEN],
                        int color_count, bool allow_repetition) {
    int count=0;
    for (const char *p=ligne; *p; ++p) {
        char c=*p;
        if (isalpha((unsigned char)c)) {
            c=(char)toupper((unsigned char)c);
            if (!caractere_couleur_valide(c, color_count)) return false;
            if (count < CODE_LEN) {
                out_code[count++] = c;
            } else return false;
        }
    }
    if (count != CODE_LEN) return false;
    if (!allow_repetition && !sans_repetition(out_code, CODE_LEN)) return false;
    return true;
}
