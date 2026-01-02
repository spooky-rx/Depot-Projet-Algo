#include <stdio.h>
#include "statistiques.h"

bool charger_stats(Stats *st, const char *chemin) {
    FILE *f = fopen(chemin, "r");
    if (!f) {
        st->games_played=0; st->games_won=0;
        st->total_tries=0; st->total_time=0.0;
        return true;
    }
    if (fscanf(f, "%lu %lu %lu %lf",
               &st->games_played, &st->games_won,
               &st->total_tries, &st->total_time) != 4) {
        st->games_played=0; st->games_won=0;
        st->total_tries=0; st->total_time=0.0;
    }
    fclose(f);
    return true;
}

bool sauvegarder_stats(const Stats *st, const char *chemin) {
    FILE *f = fopen(chemin, "w");
    if (!f) return false;
    fprintf(f, "%lu %lu %lu %.6f\n",
            st->games_played, st->games_won,
            st->total_tries, st->total_time);
    fclose(f);
    return true;
}

void afficher_stats(const Stats *st) {
    printf("\n=== Statistiques ===\n");
    printf("- Parties jouees: %lu\n", st->games_played);
    printf("- Victoires     : %lu\n", st->games_won);
    double win_rate = (st->games_played>0)
        ? (100.0 * (double)st->games_won / (double)st->games_played)
        : 0.0;
    double avg_tries = (st->games_played>0)
        ? ((double)st->total_tries / (double)st->games_played)
        : 0.0;
    double avg_time = (st->games_played>0)
        ? (st->total_time / (double)st->games_played)
        : 0.0;
    printf("- Taux de victoire: %.1f%%\n", win_rate);
    printf("- Tentatives moyennes: %.2f\n", avg_tries);
    printf("- Temps moyen par partie: %.2fs\n\n", avg_time);
}
