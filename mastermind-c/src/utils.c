#include "utils.h"
#include <stdio.h>
#include <string.h>

bool read_line(char *buffer, size_t buflen) {
    if (fgets(buffer, (int)buflen, stdin) == NULL) {
        return false;
    }
    size_t n = strlen(buffer);
    if (n > 0 && buffer[n - 1] == '\n') buffer[n - 1] = '\0';
    return true;
}

bool has_no_repetition(const char code[], int len) {
    for (int i = 0; i < len; i++) {
        for (int j = i + 1; j < len; j++) {
            if (code[i] == code[j]) return false;
        }
    }
    return true;
}
