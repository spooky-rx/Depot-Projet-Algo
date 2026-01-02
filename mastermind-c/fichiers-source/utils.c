#include <stdio.h>
#include <string.h>
#include "utils.h"

bool lire_ligne(char *buffer, size_t buflen) {
    if (fgets(buffer, (int)buflen, stdin) == NULL) return false;
    size_t n = strlen(buffer);
    if (n>0 && buffer[n-1]=='\n') buffer[n-1]='\0';
    return true;
}
