#ifndef STATISTIQUES_H
#define STATISTIQUES_H

#include <stdbool.h>
#include "types.h"

bool charger_stats(Stats *st, const char *chemin);
bool sauvegarder_stats(const Stats *st, const char *chemin);
void afficher_stats(const Stats *st);

#endif
