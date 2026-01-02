#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "types.h"

void config_defaut(GameConfig *cfg);
void preset_facile(GameConfig *cfg);
void preset_intermediaire(GameConfig *cfg);
void preset_difficile(GameConfig *cfg);
void preset_expert(GameConfig *cfg);

void afficher_configuration(const GameConfig *cfg);
void configurer_jeu(GameConfig *cfg);

#endif
