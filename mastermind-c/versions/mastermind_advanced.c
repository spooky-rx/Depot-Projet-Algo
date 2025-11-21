/*
 * Mastermind (version avancée plus) - Console en C
 *
 * Fonctionnalités:
 * - Menu: jouer (humain), jouer (IA), configurer, règles, stats, sauvegarder/charger, quitter
 * - Paramètres: 3..6 couleurs, 5..30 tentatives, répétitions ON/OFF, chrono ON/OFF (strict)
 * - Presets: facile/intermédiaire/difficile/expert
 * - IA: stratégie simple (aléatoire validée) avec feedback; base pour améliorations
 * - Sauvegarde: état de partie + historique dans save.txt
 * - Statistiques: stats.txt (victoires/défaites, tentatives moyennes, temps moyen)
 *
 * Compilation:
 *   gcc -Wall -Wextra -O2 -std=c11 mastermind_advanced.c -o mastermind_advanced
 *
 * Exécution:
 *   ./mastermind_advanced
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>

#define CODE_LEN 4
#define MAX_COLORS 6
#define MIN_COLORS 3
#define MAX_TRIES_MIN 5
#define MAX_TRIES_MAX 30

static const char GLOBAL_COLOR_SET[MAX_COLORS]   = { 'R','G','B','Y','O','P' };
static const char *GLOBAL_COLOR_NAMES[MAX_COLORS]= { "Rouge","Vert","Bleu","Jaune","Orange","Violet" };

/* =========================
   Structures de données
   ========================= */

typedef struct {
    int color_count;       // 3..6
    int max_tries;         // 5..30
    bool allow_repetition; // secret & guesses
    bool timed_mode;       // chrono par tentative
    int time_per_try_sec;  // secondes si timed_mode
} GameConfig;

typedef struct {
    char guesses[64][CODE_LEN];
    int blacks[64];
    int whites[64];
    int tries;
    char secret[CODE_LEN];
    GameConfig cfg;
    bool in_progress;
} GameState;

typedef struct {
    unsigned long games_played;
    unsigned long games_won;
    unsigned long total_tries;    // somme des tentatives utilisées
    double total_time;            // somme du temps (secondes)
} Stats;

/* =========================
   Config & presets
   ========================= */

static void set_default_config(GameConfig *cfg) {
    cfg->color_count = 6;
    cfg->max_tries = 10;
    cfg->allow_repetition = false;
    cfg->timed_mode = false;
    cfg->time_per_try_sec = 0;
}
static void set_preset_easy(GameConfig *cfg) {
    cfg->color_count = 3; cfg->max_tries = 20;
    cfg->allow_repetition = true; cfg->timed_mode = false; cfg->time_per_try_sec = 0;
}
static void set_preset_intermediate(GameConfig *cfg) {
    cfg->color_count = 4; cfg->max_tries = 15;
    cfg->allow_repetition = true; cfg->timed_mode = false; cfg->time_per_try_sec = 0;
}
static void set_preset_hard(GameConfig *cfg) {
    cfg->color_count = 5; cfg->max_tries = 10;
    cfg->allow_repetition = false; cfg->timed_mode = true; cfg->time_per_try_sec = 60;
}
static void set_preset_expert(GameConfig *cfg) {
    cfg->color_count = 6; cfg->max_tries = 5;
    cfg->allow_repetition = false; cfg->timed_mode = true; cfg->time_per_try_sec = 45;
}

/* =========================
   Utilitaires console
   ========================= */

static bool read_line(char *buffer, size_t buflen) {
    if (fgets(buffer, (int)buflen, stdin) == NULL) return false;
    size_t n = strlen(buffer);
    if (n>0 && buffer[n-1]=='\n') buffer[n-1]='\0';
    return true;
}
static void print_palette(int color_count) {
    printf("Palette:\n");
    for (int i=0;i<color_count;i++) printf("  %c = %s\n", GLOBAL_COLOR_SET[i], GLOBAL_COLOR_NAMES[i]);
}
static void print_code(const char code[CODE_LEN]) {
    for (int i=0;i<CODE_LEN;i++) printf("%c", code[i]);
}
static void banner(void) {
    printf("\n=====================================\n");
    printf("         MASTERmind - Avancé         \n");
    printf("=====================================\n\n");
}

/* =========================
   Validation / parsing
   ========================= */

static bool is_valid_color_char(char c, int color_count) {
    c = (char)toupper((unsigned char)c);
    for (int i=0;i<color_count;i++) if (GLOBAL_COLOR_SET[i]==c) return true;
    return false;
}
static bool has_no_repetition(const char code[], int len) {
    for (int i=0;i<len;i++) for (int j=i+1;j<len;j++) if (code[i]==code[j]) return false;
    return true;
}
static bool parse_guess(const char *line, char out_code[CODE_LEN],
                        int color_count, bool allow_repetition) {
    int count=0;
    for (const char *p=line; *p; ++p) {
        char c=*p;
        if (isalpha((unsigned char)c)) {
            c=(char)toupper((unsigned char)c);
            if (!is_valid_color_char(c, color_count)) return false;
            if (count<CODE_LEN) out_code[count++]=c; else return false;
        }
    }
    if (count!=CODE_LEN) return false;
    if (!allow_repetition && !has_no_repetition(out_code, CODE_LEN)) return false;
    return true;
}

/* =========================
   Secret & feedback
   ========================= */

static void generate_secret(char secret[CODE_LEN], int color_count, bool allow_repetition) {
    if (allow_repetition) {
        for (int i=0;i<CODE_LEN;i++) secret[i]=GLOBAL_COLOR_SET[rand()%color_count];
    } else {
        char pool[MAX_COLORS];
        for (int i=0;i<color_count;i++) pool[i]=GLOBAL_COLOR_SET[i];
        for (int i=color_count-1;i>0;i--) {
            int j=rand()%(i+1); char t=pool[i]; pool[i]=pool[j]; pool[j]=t;
        }
        for (int k=0;k<CODE_LEN;k++) secret[k]=pool[k];
    }
}
static void compute_feedback(const char secret[CODE_LEN], const char guess[CODE_LEN],
                             int *black, int *white) {
    *black=0; *white=0;
    bool s_used[CODE_LEN]={0}, g_used[CODE_LEN]={0};
    for (int i=0;i<CODE_LEN;i++) if (secret[i]==guess[i]) { (*black)++; s_used[i]=1; g_used[i]=1; }
    int sc[256]={0}, gc[256]={0};
    for (int i=0;i<CODE_LEN;i++) {
        if (!s_used[i]) sc[(unsigned char)secret[i]]++;
        if (!g_used[i]) gc[(unsigned char)guess[i]]++;
    }
    int matches=0;
    for (int c=0;c<256;c++) if (sc[c]>0 && gc[c]>0) matches += (sc[c]<gc[c]?sc[c]:gc[c]);
    *white=matches;
}

/* =========================
   Timer strict par tentative
   ========================= */

static bool timed_get_guess(char out_code[CODE_LEN], int color_count, bool allow_repetition,
                            int time_limit_sec) {
    // Mesure du temps de saisie; si dépassement, tentative annulée
    time_t start=time(NULL);
    char line[256];
    if (!read_line(line, sizeof(line))) return false;
    time_t end=time(NULL);
    double elapsed=difftime(end, start);
    if (elapsed > (double)time_limit_sec) {
        printf("Temps depasse (%.0fs > %ds). Tentative annulee.\n", elapsed, time_limit_sec);
        return false;
    }
    return parse_guess(line, out_code, color_count, allow_repetition);
}

/* =========================
   Sauvegarde / chargement
   ========================= */

static bool save_game(const GameState *gs, const char *path) {
    FILE *f=fopen(path, "w");
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
                i+1, gs->guesses[i][0], gs->guesses[i][1], gs->guesses[i][2], gs->guesses[i][3],
                gs->blacks[i], gs->whites[i]);
    }
    fclose(f);
    return true;
}
static bool load_game(GameState *gs, const char *path) {
    FILE *f=fopen(path, "r");
    if (!f) return false;
    memset(gs, 0, sizeof(*gs));
    gs->in_progress=true;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "color_count=%d", &gs->cfg.color_count)==1) continue;
        if (sscanf(line, "max_tries=%d", &gs->cfg.max_tries)==1) continue;
        int b;
        if (sscanf(line, "allow_repetition=%d", &b)==1) { gs->cfg.allow_repetition = (b!=0); continue; }
        if (sscanf(line, "timed_mode=%d", &b)==1) { gs->cfg.timed_mode = (b!=0); continue; }
        if (sscanf(line, "time_per_try_sec=%d", &gs->cfg.time_per_try_sec)==1) continue;
        if (sscanf(line, "tries=%d", &gs->tries)==1) continue;
        if (sscanf(line, "secret=%c%c%c%c", &gs->secret[0], &gs->secret[1], &gs->secret[2], &gs->secret[3])==4) continue;

        int idx, black, white; char g0,g1,g2,g3;
        if (sscanf(line, "guess%d=%c%c%c%c black=%d white=%d",
                   &idx, &g0,&g1,&g2,&g3, &black, &white)==7) {
            int i=idx-1;
            gs->guesses[i][0]=g0; gs->guesses[i][1]=g1; gs->guesses[i][2]=g2; gs->guesses[i][3]=g3;
            gs->blacks[i]=black; gs->whites[i]=white;
        }
    }
    fclose(f);
    return true;
}

/* =========================
   Statistiques persistantes
   ========================= */

static bool load_stats(Stats *st, const char *path) {
    FILE *f=fopen(path, "r");
    if (!f) { st->games_played=0; st->games_won=0; st->total_tries=0; st->total_time=0.0; return true; }
    if (fscanf(f, "%lu %lu %lu %lf", &st->games_played, &st->games_won, &st->total_tries, &st->total_time)!=4) {
        st->games_played=0; st->games_won=0; st->total_tries=0; st->total_time=0.0;
    }
    fclose(f);
    return true;
}
static bool save_stats(const Stats *st, const char *path) {
    FILE *f=fopen(path, "w");
    if (!f) return false;
    fprintf(f, "%lu %lu %lu %.6f\n", st->games_played, st->games_won, st->total_tries, st->total_time);
    fclose(f);
    return true;
}
static void print_stats(const Stats *st) {
    printf("\n=== Statistiques ===\n");
    printf("- Parties jouees: %lu\n", st->games_played);
    printf("- Victoires     : %lu\n", st->games_won);
    double win_rate = (st->games_played>0) ? (100.0 * (double)st->games_won / (double)st->games_played) : 0.0;
    printf("- Taux de victoire: %.1f%%\n", win_rate);
    double avg_tries = (st->games_played>0) ? ((double)st->total_tries / (double)st->games_played) : 0.0;
    printf("- Tentatives moyennes: %.2f\n", avg_tries);
    double avg_time = (st->games_played>0) ? (st->total_time / (double)st->games_played) : 0.0;
    printf("- Temps moyen par partie: %.2fs\n\n", avg_time);
}

/* =========================
   IA (stratégie simple)
   ========================= */

static void ai_generate_random_guess(char guess[CODE_LEN], const GameConfig *cfg) {
    if (cfg->allow_repetition) {
        for (int i=0;i<CODE_LEN;i++) guess[i]=GLOBAL_COLOR_SET[rand()%cfg->color_count];
    } else {
        // tire CODE_LEN distincts de la palette
        int used[MAX_COLORS]={0};
        int k=0;
        while (k<CODE_LEN) {
            int idx=rand()%cfg->color_count;
            if (!used[idx]) { used[idx]=1; guess[k++]=GLOBAL_COLOR_SET[idx]; }
        }
    }
}

/* =========================
   Affichage historique
   ========================= */

static void print_history(const GameState *gs) {
    printf("Historique des essais:\n");
    for (int i=0;i<gs->tries;i++) {
        printf("  %2d) ", i+1);
        print_code(gs->guesses[i]);
        printf("  => ●: %d, ○: %d\n", gs->blacks[i], gs->whites[i]);
    }
}

/* =========================
   Partie: Humain
   ========================= */

static void play_human(GameConfig cfg, Stats *st) {
    GameState gs = {0};
    gs.cfg = cfg; gs.in_progress=true;
    srand((unsigned int)time(NULL));

    banner();
    print_palette(cfg.color_count);
    printf("Objectif: devinez le code (%d lettres) en %d tentatives.\n", CODE_LEN, cfg.max_tries);
    printf("Options: repetitions %s, chrono %s",
           cfg.allow_repetition?"ON":"OFF",
           cfg.timed_mode?"ON":"OFF");
    if (cfg.timed_mode) printf(" (%ds)", cfg.time_per_try_sec);
    printf("\nFeedback: ● noir (bien place), ○ blanc (bonne couleur, mauvaise position)\n\n");

    generate_secret(gs.secret, cfg.color_count, cfg.allow_repetition);

    time_t start_part = time(NULL);

    while (gs.tries < cfg.max_tries) {
        printf("Tentative %d/%d - Votre proposition: ", gs.tries+1, cfg.max_tries);

        char guess[CODE_LEN];
        bool ok=false;

        if (cfg.timed_mode) {
            ok = timed_get_guess(guess, cfg.color_count, cfg.allow_repetition, cfg.time_per_try_sec);
        } else {
            char line[256];
            if (!read_line(line, sizeof(line))) { printf("Lecture invalide.\n"); continue; }
            ok = parse_guess(line, guess, cfg.color_count, cfg.allow_repetition);
        }

        if (!ok) {
            printf("Entree invalide ou hors temps. Rappel: %d lettres parmi ", CODE_LEN);
            for (int i=0;i<cfg.color_count;i++) {
                printf("%c%s", GLOBAL_COLOR_SET[i], (i+1<cfg.color_count)?" ":"");
            }
            printf(", %s repetition.\n\n", cfg.allow_repetition?"avec":"sans");
            // Option: permettre sauvegarde à tout moment
            printf("Tapez 'save' pour sauvegarder la partie, ou reessayez.\n");
            continue;
        }

        int black=0, white=0;
        compute_feedback(gs.secret, guess, &black, &white);

        memcpy(gs.guesses[gs.tries], guess, CODE_LEN);
        gs.blacks[gs.tries]=black; gs.whites[gs.tries]=white;
        gs.tries++;

        printf("Vous avez propose: "); print_code(guess);
        printf("  => ●: %d, ○: %d\n", black, white);
        print_history(&gs);
        printf("\n");

        if (black==CODE_LEN) {
            time_t end_part = time(NULL);
            double elapsed = difftime(end_part, start_part);
            printf("Bravo ! Code trouve en %d tentative%s.\n", gs.tries, gs.tries>1?"s":"");
            printf("Code secret: "); print_code(gs.secret); printf("\n");
            // Stats
            st->games_played++; st->games_won++; st->total_tries += gs.tries; st->total_time += elapsed;
            save_stats(st, "stats.txt");
            gs.in_progress=false;
            return;
        }

        // Sauvegarde à la volée (option)
        printf("Commande (enter pour continuer) [save/quit]: ");
        char cmd[32]; if (read_line(cmd, sizeof(cmd))) {
            if (strcmp(cmd,"save")==0) {
                if (save_game(&gs, "save.txt")) printf("Partie sauvegardee.\n");
                else printf("Echec sauvegarde.\n");
            } else if (strcmp(cmd,"quit")==0) {
                printf("Abandon de la partie.\n");
                break;
            }
        }
    }

    time_t end_part = time(NULL);
    double elapsed = difftime(end_part, start_part);
    printf("Dommage ! Vous n'avez pas trouve le code.\n");
    printf("Le code secret etait: "); print_code(gs.secret); printf("\n");
    st->games_played++; st->total_tries += gs.tries; st->total_time += elapsed;
    save_stats(st, "stats.txt");
}

/* =========================
   Partie: IA
   ========================= */

static void play_ai(GameConfig cfg, Stats *st) {
    banner();
    printf("[Mode IA] L'ordinateur tente de deviner.\n");
    print_palette(cfg.color_count);

    char secret[CODE_LEN];
    generate_secret(secret, cfg.color_count, cfg.allow_repetition);

    printf("Secret: **** (masque)\n");
    printf("Options: repetitions %s, chrono %s\n\n",
           cfg.allow_repetition?"ON":"OFF", cfg.timed_mode?"ON":"OFF");

    int tries=0;
    time_t start_part = time(NULL);

    while (tries < cfg.max_tries) {
        char guess[CODE_LEN];
        ai_generate_random_guess(guess, &cfg);

        int black=0, white=0;
        compute_feedback(secret, guess, &black, &white);
        tries++;

        printf("IA Tentative %d/%d: ", tries, cfg.max_tries); print_code(guess);
        printf("  => ●: %d, ○: %d\n", black, white);

        if (black==CODE_LEN) {
            time_t end_part = time(NULL);
            double elapsed = difftime(end_part, start_part);
            printf("IA a trouve le code en %d tentative%s.\n", tries, tries>1?"s":"");
            printf("Code secret: "); print_code(secret); printf("\n");
            // Stats (on enregistre comme une partie jouée non-humaine? Ici on n'incrémente pas games_won humain)
            st->games_played++; st->total_tries += tries; st->total_time += elapsed;
            save_stats(st, "stats.txt");
            return;
        }
    }

    time_t end_part = time(NULL);
    double elapsed = difftime(end_part, start_part);
    printf("IA n'a pas trouve le code.\n");
    printf("Le code secret etait: "); print_code(secret); printf("\n");
    st->games_played++; st->total_tries += tries; st->total_time += elapsed;
    save_stats(st, "stats.txt");
}

/* =========================
   Règles & Config
   ========================= */

static void print_rules(void) {
    printf("\n=== Règles & Options ===\n");
    printf("- Code: %d lettres parmi 3..6 couleurs, selon la configuration.\n", CODE_LEN);
    printf("- Tentatives: 5..30.\n");
    printf("- Feedback: ● noir (bien place), ○ blanc (bonne couleur, mauvaise position).\n");
    printf("- Repetitions: ON/OFF (affecte secret et saisie).\n");
    printf("- Chronometre strict: si temps depasse, tentative annulee.\n");
    printf("- Presets: facile/intermediaire/difficile/expert.\n");
    printf("- Modes: Humain vs Code, IA qui devine.\n");
    printf("- Sauvegarde: 'save.txt' et reprise via menu.\n");
    printf("- Statistiques: 'stats.txt' (victoires, moyennes, temps).\n\n");
}
static void print_config(const GameConfig *cfg) {
    printf("\n=== Configuration ===\n");
    printf("Couleurs: %d\n", cfg->color_count);
    printf("Tentatives: %d\n", cfg->max_tries);
    printf("Repetitions: %s\n", cfg->allow_repetition?"ON":"OFF");
    printf("Chronometre: %s", cfg->timed_mode?"ON":"OFF");
    if (cfg->timed_mode) printf(" (%ds)", cfg->time_per_try_sec);
    printf("\n\n");
}
static int ask_int(const char *prompt, int minv, int maxv) {
    char buf[128]; int val;
    while (1) {
        printf("%s [%d..%d]: ", prompt, minv, maxv);
        if (!read_line(buf, sizeof(buf))) continue;
        if (sscanf(buf, "%d", &val)==1 && val>=minv && val<=maxv) return val;
        printf("Valeur invalide.\n");
    }
}
static bool ask_yes_no(const char *prompt) {
    char buf[64];
    while (1) {
        printf("%s (o/n): ", prompt);
        if (!read_line(buf, sizeof(buf))) continue;
        if (buf[0]=='o'||buf[0]=='O'||buf[0]=='y'||buf[0]=='Y') return true;
        if (buf[0]=='n'||buf[0]=='N') return false;
        printf("Reponse invalide.\n");
    }
}
static void configure_game(GameConfig *cfg) {
    print_config(cfg);
    printf("1) Facile\n2) Intermediaire\n3) Difficile\n4) Expert\n5) Personnaliser\n0) Retour\nChoix: ");
    char line[64]; if (!read_line(line, sizeof(line))) return;
    int c=atoi(line);
    switch (c) {
        case 1: set_preset_easy(cfg); break;
        case 2: set_preset_intermediate(cfg); break;
        case 3: set_preset_hard(cfg); break;
        case 4: set_preset_expert(cfg); break;
        case 5: {
            cfg->color_count = ask_int("Nombre de couleurs", MIN_COLORS, MAX_COLORS);
            cfg->max_tries   = ask_int("Nombre de tentatives", MAX_TRIES_MIN, MAX_TRIES_MAX);
            cfg->allow_repetition = ask_yes_no("Autoriser les repetitions ?");
            cfg->timed_mode = ask_yes_no("Activer le chronometre strict ?");
            cfg->time_per_try_sec = cfg->timed_mode ? ask_int("Temps par tentative (s)", 10, 300) : 0;
            break;
        }
        default: break;
    }
    print_config(cfg);
    printf("Configuration mise a jour.\n");
}

/* =========================
   Charger une partie
   ========================= */

static void resume_game(Stats *st) {
    GameState gs;
    if (!load_game(&gs, "save.txt") || !gs.in_progress) {
        printf("Aucune sauvegarde disponible.\n");
        return;
    }
    banner();
    printf("Reprise de partie. Tentatives deja effectuees: %d/%d\n", gs.tries, gs.cfg.max_tries);
    print_palette(gs.cfg.color_count);
    print_history(&gs);
    printf("\n");

    time_t start_part = time(NULL);

    while (gs.tries < gs.cfg.max_tries) {
        printf("Tentative %d/%d - Votre proposition: ", gs.tries+1, gs.cfg.max_tries);

        char guess[CODE_LEN]; bool ok=false;
        if (gs.cfg.timed_mode) ok = timed_get_guess(guess, gs.cfg.color_count, gs.cfg.allow_repetition, gs.cfg.time_per_try_sec);
        else {
            char line[256]; if (!read_line(line, sizeof(line))) { printf("Lecture invalide.\n"); continue; }
            ok = parse_guess(line, guess, gs.cfg.color_count, gs.cfg.allow_repetition);
        }
        if (!ok) { printf("Entree invalide ou hors temps.\n"); continue; }

        int black=0, white=0;
        compute_feedback(gs.secret, guess, &black, &white);

        memcpy(gs.guesses[gs.tries], guess, CODE_LEN);
        gs.blacks[gs.tries]=black; gs.whites[gs.tries]=white;
        gs.tries++;

        printf("Vous avez propose: "); print_code(guess);
        printf("  => ●: %d, ○: %d\n", black, white);
        print_history(&gs);
        printf("\n");

        if (black==CODE_LEN) {
            time_t end_part = time(NULL);
            double elapsed = difftime(end_part, start_part);
            printf("Bravo ! Code trouve en %d tentative%s.\n", gs.tries, gs.tries>1?"s":"");
            printf("Code secret: "); print_code(gs.secret); printf("\n");
            st->games_played++; st->games_won++; st->total_tries += gs.tries; st->total_time += elapsed;
            save_stats(st, "stats.txt");
            // Nettoyage de la sauvegarde (optionnel): on réinitialise
            FILE *f=fopen("save.txt","w"); if (f){ fclose(f); }
            return;
        }

        // Mise à jour de la sauvegarde continue
        save_game(&gs, "save.txt");
    }

    time_t end_part = time(NULL);
    double elapsed = difftime(end_part, start_part);
    printf("Dommage ! Vous n'avez pas trouve le code.\n");
    printf("Le code secret etait: "); print_code(gs.secret); printf("\n");
    st->games_played++; st->total_tries += gs.tries; st->total_time += elapsed;
    save_stats(st, "stats.txt");
}

/* =========================
   Menu principal
   ========================= */

static void menu_loop(void) {
    GameConfig cfg; set_default_config(&cfg);
    Stats stats; load_stats(&stats, "stats.txt");
    srand((unsigned int)time(NULL));

    while (1) {
        printf("=== Menu Principal ===\n");
        printf("1) Jouer (Humain)\n");
        printf("2) Jouer (IA)\n");
        printf("3) Configurer\n");
        printf("4) Afficher les regles\n");
        printf("5) Afficher les statistiques\n");
        printf("6) Reprendre une partie (charger)\n");
        printf("0) Quitter\n");
        printf("Choix: ");

        char line[64]; if (!read_line(line, sizeof(line))) continue;
        int choice=atoi(line);

        switch (choice) {
            case 1: play_human(cfg, &stats); break;
            case 2: play_ai(cfg, &stats); break;
            case 3: configure_game(&cfg); break;
            case 4: print_rules(); break;
            case 5: print_stats(&stats); break;
            case 6: resume_game(&stats); break;
            case 0: printf("Au revoir !\n"); return;
            default: printf("Choix invalide.\n"); break;
        }
        printf("\n");
    }
}

int main(void) {
    menu_loop();
    return 0;
}


