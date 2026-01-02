#ifndef SAUVEGARDE_H
#define SAUVEGARDE_H

#include <stdbool.h>
#include "types.h"

bool sauvegarder_partie(const GameState *gs, const char *chemin);
bool charger_partie(GameState *gs, const char *chemin);

#endif
