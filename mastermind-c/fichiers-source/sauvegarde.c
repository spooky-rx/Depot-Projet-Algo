#include <stdio.h>
#include <string.h>
#include "sauvegarde.h"

bool sauvegarder_partie(const GameState *gs, const char *chemin) {
    FILE *f = fopen(chemin, "w");
    if (!f) return false;
    fprintf(f, "color_count=%d\n", gs->cfg.color_count);
    fprintf(f, "max_tries=%d\n", gs->cfg.max_tries);
    fprintf(f, "allow_repetition=%d\n", gs->cfg.allow_repetition?1:0);
    fprintf(f, "timed_mode=%d\n", gs->cfg.timed_mode?1:0);
    fprintf(f, "time_per_try_sec=%d\n", gs->cfg.time_per_try_sec);
    fprintf(f, "tries=%d\n", gs->tries);
    fprintf(f, "secret=%c%c%c%c\n", gs->secret[0], gs->secret[1], gs->secret[2], gs->secret[3]);
    for (int i=0;i<gs->tries;i++) {
        fprintf(f, "guess%d=%c%c%c%c black=%d white=%d\n",
                i+1, gs->guesses[i][0], gs->guesses[i][1],
                gs->guesses[i][2], gs->guesses[i][3],
                gs->blacks[i], gs->whites[i]);
    }
    fclose(f);
    return true;
}

bool charger_partie(GameState *gs, const char *chemin) {
    FILE *f = fopen(chemin, "r");
    if (!f) return false;
    memset(gs, 0, sizeof(*gs));
    gs->in_progress = true;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "color_count=%d", &gs->cfg.color_count)==1) continue;
        if (sscanf(line, "max_tries=%d", &gs->cfg.max_tries)==1) continue;
        int b;
        if (sscanf(line, "allow_repetition=%d", &b)==1) { gs->cfg.allow_repetition=(b!=0); continue; }
        if (sscanf(line, "timed_mode=%d", &b)==1) { gs->cfg.timed_mode=(b!=0); continue; }
        if (sscanf(line, "time_per_try_sec=%d", &gs->cfg.time_per_try_sec)==1) continue;
        if (sscanf(line, "tries=%d", &gs->tries)==1) continue;
        if (sscanf(line, "secret=%c%c%c%c",
                   &gs->secret[0], &gs->secret[1],
                   &gs->secret[2], &gs->secret[3])==4) continue;

        int idx, black, white; char g0,g1,g2,g3;
        if (sscanf(line, "guess%d=%c%c%c%c black=%d white=%d",
                   &idx, &g0,&g1,&g2,&g3, &black,&white)==7) {
            int i=idx-1;
            gs->guesses[i][0]=g0; gs->guesses[i][1]=g1;
            gs->guesses[i][2]=g2; gs->guesses[i][3]=g3;
            gs->blacks[i]=black; gs->whites[i]=white;
        }
    }
    fclose(f);
    return true;
}
