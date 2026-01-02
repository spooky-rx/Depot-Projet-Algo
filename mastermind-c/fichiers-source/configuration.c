#include <stdio.h>
#include <stdlib.h>
#include "configuration.h"
#include "utils.h"
#include "types.h"

void config_defaut(GameConfig *cfg) {
    cfg->color_count = 6;
    cfg->max_tries = 10;
    cfg->allow_repetition = false;
    cfg->timed_mode = false;
    cfg->time_per_try_sec = 0;
}
void preset_facile(GameConfig *cfg) {
    cfg->color_count = 3; cfg->max_tries = 20;
    cfg->allow_repetition = true; cfg->timed_mode = false; cfg->time_per_try_sec = 0;
}
void preset_intermediaire(GameConfig *cfg) {
    cfg->color_count = 4; cfg->max_tries = 15;
    cfg->allow_repetition = true; cfg->timed_mode = false; cfg->time_per_try_sec = 0;
}
void preset_difficile(GameConfig *cfg) {
    cfg->color_count = 5; cfg->max_tries = 10;
    cfg->allow_repetition = false; cfg->timed_mode = true; cfg->time_per_try_sec = 60;
}
void preset_expert(GameConfig *cfg) {
    cfg->color_count = 6; cfg->max_tries = 5;
    cfg->allow_repetition = false; cfg->timed_mode = true; cfg->time_per_try_sec = 45;
}

void afficher_configuration(const GameConfig *cfg) {
    printf("\n=== Configuration ===\n");
    printf("Couleurs: %d\n", cfg->color_count);
    printf("Tentatives: %d\n", cfg->max_tries);
    printf("Repetitions: %s\n", cfg->allow_repetition ? "ON" : "OFF");
    printf("Chronometre: %s", cfg->timed_mode ? "ON" : "OFF");
    if (cfg->timed_mode) printf(" (%ds)", cfg->time_per_try_sec);
    printf("\n\n");
}

static int demander_entier(const char *prompt, int minv, int maxv) {
    char buf[128]; int val;
    while (1) {
        printf("%s [%d..%d]: ", prompt, minv, maxv);
        if (!lire_ligne(buf, sizeof(buf))) continue;
        if (sscanf(buf, "%d", &val)==1 && val>=minv && val<=maxv) return val;
        printf("Valeur invalide.\n");
    }
}
static bool demander_oui_non(const char *prompt) {
    char buf[64];
    while (1) {
        printf("%s (o/n): ", prompt);
        if (!lire_ligne(buf, sizeof(buf))) continue;
        if (buf[0]=='o'||buf[0]=='O'||buf[0]=='y'||buf[0]=='Y') return true;
        if (buf[0]=='n'||buf[0]=='N') return false;
        printf("Reponse invalide.\n");
    }
}

void configurer_jeu(GameConfig *cfg) {
    afficher_configuration(cfg);
    printf("1) Facile\n2) Intermediaire\n3) Difficile\n4) Expert\n5) Personnaliser\n0) Retour\nChoix: ");
    char line[64]; if (!lire_ligne(line, sizeof(line))) return;
    int c = atoi(line);
    switch (c) {
        case 1: preset_facile(cfg); break;
        case 2: preset_intermediaire(cfg); break;
        case 3: preset_difficile(cfg); break;
        case 4: preset_expert(cfg); break;
        case 5:
            cfg->color_count = demander_entier("Nombre de couleurs", MIN_COLORS, MAX_COLORS);
            cfg->max_tries   = demander_entier("Nombre de tentatives", MAX_TRIES_MIN, MAX_TRIES_MAX);
            cfg->allow_repetition = demander_oui_non("Autoriser les repetitions ?");
            cfg->timed_mode = demander_oui_non("Activer le chronometre strict ?");
            cfg->time_per_try_sec = cfg->timed_mode ? demander_entier("Temps par tentative (s)", 10, 300) : 0;
            break;
        default: break;
    }
    afficher_configuration(cfg);
    printf("Configuration mise a jour.\n");
}
