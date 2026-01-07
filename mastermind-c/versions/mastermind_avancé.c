#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>

/* Paramètres “globaux” du jeu */
#define CODE_LEN 4
#define MAX_COLORS 6
#define MIN_COLORS 3
#define MAX_TRIES_MIN 5
#define MAX_TRIES_MAX 30

/* La palette: on joue avec des lettres (R,G,B,...) + des noms pour l’affichage */
static const char GLOBAL_COLOR_SET[MAX_COLORS]   = { 'R','G','B','Y','O','P' };
static const char *GLOBAL_COLOR_NAMES[MAX_COLORS]= { "Rouge","Vert","Bleu","Jaune","Orange","Violet" };

/* -------------------------
   Structures de données
   ------------------------- */

/* Configuration d’une partie (ce que tu règles dans “Configurer”) */
typedef struct {
    int color_count;       // combien de couleurs on autorise (3..6)
    int max_tries;         // nb max de tentatives (5..30)
    bool allow_repetition; // si on autorise les doublons dans le code et les guesses
    bool timed_mode;       // mode chrono strict (si tu dépasses → tentative “perdue”)
    int time_per_try_sec;  // limite de temps par tentative si timed_mode
} GameConfig;

/* État complet d’une partie (utile pour jouer + sauvegarder/reprendre) */
typedef struct {
    char guesses[64][CODE_LEN]; // historique des propositions (64 c’est large)
    int blacks[64];             // pions noirs par tentative
    int whites[64];             // pions blancs par tentative
    int tries;                  // combien de tentatives déjà faites
    char secret[CODE_LEN];      // le code secret en mémoire
    GameConfig cfg;             // la config qui a servi à lancer la partie
    bool in_progress;           // pratique pour savoir si save “valide”
} GameState;

/* Stats cumulées et sauvegardées dans stats.txt */
typedef struct {
    unsigned long games_played;
    unsigned long games_won;
    unsigned long total_tries;  // pour faire une moyenne
    double total_time;          // pour faire une moyenne
} Stats;

/* -------------------------
   Presets / configuration
   ------------------------- */

/* Valeurs “normales” si l’utilisateur ne touche à rien */
static void set_default_config(GameConfig *cfg) {
    cfg->color_count = 6;
    cfg->max_tries = 10;
    cfg->allow_repetition = false;
    cfg->timed_mode = false;
    cfg->time_per_try_sec = 0;
}

/* Presets: juste des configs prêtes à l’emploi */
static void set_preset_easy(GameConfig *cfg) {
    cfg->color_count = 3;
    cfg->max_tries = 20;
    cfg->allow_repetition = true;
    cfg->timed_mode = false;
    cfg->time_per_try_sec = 0;
}
static void set_preset_intermediate(GameConfig *cfg) {
    cfg->color_count = 4;
    cfg->max_tries = 15;
    cfg->allow_repetition = true;
    cfg->timed_mode = false;
    cfg->time_per_try_sec = 0;
}
static void set_preset_hard(GameConfig *cfg) {
    cfg->color_count = 5;
    cfg->max_tries = 10;
    cfg->allow_repetition = false;
    cfg->timed_mode = true;
    cfg->time_per_try_sec = 60;
}
static void set_preset_expert(GameConfig *cfg) {
    cfg->color_count = 6;
    cfg->max_tries = 5;
    cfg->allow_repetition = false;
    cfg->timed_mode = true;
    cfg->time_per_try_sec = 45;
}

/* -------------------------
   Entrées utilisateur / parsing
   ------------------------- */

/* read_line: lecture propre d’une ligne (évite scanf qui est chiant avec les espaces/newlines) */
static bool read_line(char *buffer, size_t buflen) {
    if (fgets(buffer, (int)buflen, stdin) == NULL) return false;
    size_t n = strlen(buffer);
    if (n > 0 && buffer[n-1] == '\n') buffer[n-1] = '\0';
    return true;
}

/* Vérifie si une lettre fait partie de la palette autorisée (selon color_count) */
static bool is_valid_color_char(char c, int color_count) {
    c = (char)toupper((unsigned char)c);
    for (int i = 0; i < color_count; i++)
        if (GLOBAL_COLOR_SET[i] == c) return true;
    return false;
}

/* Petit helper: détecte les doublons (utile quand repetition = OFF) */
static bool has_no_repetition(const char code[], int len) {
    for (int i = 0; i < len; i++)
        for (int j = i+1; j < len; j++)
            if (code[i] == code[j]) return false;
    return true;
}

/*
  parse_guess:
  - lit une ligne libre (l’utilisateur peut taper "R G B Y" ou "RGBY" etc.)
  - garde uniquement les lettres
  - vérifie que c’est bien 4 lettres valides
  - applique la règle répétition ON/OFF
*/
static bool parse_guess(const char *line, char out_code[CODE_LEN],
                        int color_count, bool allow_repetition) {
    int count = 0;

    for (const char *p = line; *p; ++p) {
        char c = *p;

        if (isalpha((unsigned char)c)) {                 // on ignore tout sauf les lettres
            c = (char)toupper((unsigned char)c);

            if (!is_valid_color_char(c, color_count))    // lettre inconnue → direct non
                return false;

            if (count < CODE_LEN)                        // on remplit le code lettre par lettre
                out_code[count++] = c;
            else                                         // trop de lettres → non (ex: "RGBYOO")
                return false;
        }
    }

    if (count != CODE_LEN)                               // pas assez de lettres → non
        return false;

    if (!allow_repetition && !has_no_repetition(out_code, CODE_LEN))
        return false;

    return true;
}

/* -------------------------
   Génération du secret + feedback (le cœur du Mastermind)
   ------------------------- */

/*
  generate_secret:
  - si répétitions ON: simple rand()%color_count pour chaque position
  - si répétitions OFF: on mélange un “pool” de couleurs (shuffle) et on prend les 4 premières
*/
static void generate_secret(char secret[CODE_LEN], int color_count, bool allow_repetition) {
    if (allow_repetition) {
        for (int i = 0; i < CODE_LEN; i++)
            secret[i] = GLOBAL_COLOR_SET[rand() % color_count];
    } else {
        char pool[MAX_COLORS];
        for (int i = 0; i < color_count; i++) pool[i] = GLOBAL_COLOR_SET[i];

        // shuffle Fisher-Yates (classique, efficace)
        for (int i = color_count - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            char t = pool[i]; pool[i] = pool[j]; pool[j] = t;
        }

        for (int k = 0; k < CODE_LEN; k++)
            secret[k] = pool[k];
    }
}

/*
  compute_feedback:
  But: calculer (black, white)
  - black = bonnes couleurs à la bonne position
  - white = bonnes couleurs mais mauvaise position (sans double-compter)

  Méthode:
  1) on marque les positions exactes (black) et on “retire” ces cases
  2) on compte les occurrences restantes de chaque couleur dans secret et guess
  3) pour chaque couleur, white += min(count_secret, count_guess)
*/
static void compute_feedback(const char secret[CODE_LEN], const char guess[CODE_LEN],
                             int *black, int *white) {
    *black = 0;
    *white = 0;

    bool s_used[CODE_LEN] = {0};
    bool g_used[CODE_LEN] = {0};

    // 1) blacks = match exact position
    for (int i = 0; i < CODE_LEN; i++) {
        if (secret[i] == guess[i]) {
            (*black)++;
            s_used[i] = 1;
            g_used[i] = 1;
        }
    }

    // 2) compter les couleurs restantes
    int sc[256] = {0};
    int gc[256] = {0};

    for (int i = 0; i < CODE_LEN; i++) {
        if (!s_used[i]) sc[(unsigned char)secret[i]]++;
        if (!g_used[i]) gc[(unsigned char)guess[i]]++;
    }

    // 3) whites = somme des min(sc[c], gc[c])
    int matches = 0;
    for (int c = 0; c < 256; c++) {
        if (sc[c] > 0 && gc[c] > 0)
            matches += (sc[c] < gc[c] ? sc[c] : gc[c]);
    }

    *white = matches;
}

/* -------------------------
   Mode chrono strict
   ------------------------- */

/*
  timed_get_guess:
  idée “simple” : on mesure le temps entre avant/après read_line
  Limite: en console, si l’utilisateur dépasse, on ne peut pas “interrompre” fgets.
  Donc ça ne bloque pas l’utilisateur… ça annule juste la tentative après coup.
*/
static bool timed_get_guess(char out_code[CODE_LEN], int color_count, bool allow_repetition,
                            int time_limit_sec) {
    time_t start = time(NULL);

    char line[256];
    if (!read_line(line, sizeof(line))) return false;

    time_t end = time(NULL);
    double elapsed = difftime(end, start);

    if (elapsed > (double)time_limit_sec) {
        printf("Temps depasse (%.0fs > %ds). Tentative annulee.\n", elapsed, time_limit_sec);
        return false;
    }

    return parse_guess(line, out_code, color_count, allow_repetition);
}

/* -------------------------
   Sauvegarde / chargement
   ------------------------- */

/*
  save_game:
  écrit la config + le secret + l’historique dans un fichier texte.
  C’est pas un format binaire: pratique à déboguer à la main.
*/
static bool save_game(const GameState *gs, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return false;

    fprintf(f, "color_count=%d\n", gs->cfg.color_count);
    fprintf(f, "max_tries=%d\n", gs->cfg.max_tries);
    fprintf(f, "allow_repetition=%d\n", gs->cfg.allow_repetition ? 1 : 0);
    fprintf(f, "timed_mode=%d\n", gs->cfg.timed_mode ? 1 : 0);
    fprintf(f, "time_per_try_sec=%d\n", gs->cfg.time_per_try_sec);
    fprintf(f, "tries=%d\n", gs->tries);

    // On sauve même le secret: indispensable pour reprendre exactement la partie
    fprintf(f, "secret=%c%c%c%c\n", gs->secret[0], gs->secret[1], gs->secret[2], gs->secret[3]);

    // Historique des tentatives avec leur feedback
    for (int i = 0; i < gs->tries; i++) {
        fprintf(f, "guess%d=%c%c%c%c black=%d white=%d\n",
                i+1,
                gs->guesses[i][0], gs->guesses[i][1], gs->guesses[i][2], gs->guesses[i][3],
                gs->blacks[i], gs->whites[i]);
    }

    fclose(f);
    return true;
}

/*
  load_game:
  relit save.txt ligne par ligne et reconstruit GameState
  Note: c’est tolérant: si les lignes sont dans un ordre différent, ça marche quand même (grâce aux sscanf).
*/
static bool load_game(GameState *gs, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return false;

    memset(gs, 0, sizeof(*gs));
    gs->in_progress = true;

    char line[256];

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "color_count=%d", &gs->cfg.color_count) == 1) continue;
        if (sscanf(line, "max_tries=%d", &gs->cfg.max_tries) == 1) continue;

        int b;
        if (sscanf(line, "allow_repetition=%d", &b) == 1) { gs->cfg.allow_repetition = (b != 0); continue; }
        if (sscanf(line, "timed_mode=%d", &b) == 1) { gs->cfg.timed_mode = (b != 0); continue; }

        if (sscanf(line, "time_per_try_sec=%d", &gs->cfg.time_per_try_sec) == 1) continue;
        if (sscanf(line, "tries=%d", &gs->tries) == 1) continue;

        if (sscanf(line, "secret=%c%c%c%c",
                   &gs->secret[0], &gs->secret[1], &gs->secret[2], &gs->secret[3]) == 4) continue;

        // Partie “historique”
        int idx, black, white;
        char g0,g1,g2,g3;

        if (sscanf(line, "guess%d=%c%c%c%c black=%d white=%d",
                   &idx, &g0, &g1, &g2, &g3, &black, &white) == 7) {
            int i = idx - 1;
            gs->guesses[i][0] = g0; gs->guesses[i][1] = g1; gs->guesses[i][2] = g2; gs->guesses[i][3] = g3;
            gs->blacks[i] = black;
            gs->whites[i] = white;
        }
    }

    fclose(f);
    return true;
}

/* -------------------------
   Stats (persistantes)
   ------------------------- */

/*
  load_stats:
  si stats.txt n’existe pas → on démarre à zéro (normal).
  sinon on lit 4 valeurs: played, won, total_tries, total_time
*/
static bool load_stats(Stats *st, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        st->games_played = 0;
        st->games_won = 0;
        st->total_tries = 0;
        st->total_time = 0.0;
        return true;
    }

    if (fscanf(f, "%lu %lu %lu %lf",
               &st->games_played, &st->games_won, &st->total_tries, &st->total_time) != 4) {
        // fichier corrompu ou format inattendu → reset “safe”
        st->games_played = 0;
        st->games_won = 0;
        st->total_tries = 0;
        st->total_time = 0.0;
    }

    fclose(f);
    return true;
}

/* Sauvegarde simple des stats (une ligne) */
static bool save_stats(const Stats *st, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return false;

    fprintf(f, "%lu %lu %lu %.6f\n",
            st->games_played, st->games_won, st->total_tries, st->total_time);

    fclose(f);
    return true;
}

/* Affichage des stats avec calculs de moyennes */
static void print_stats(const Stats *st) {
    printf("\n=== Statistiques ===\n");
    printf("- Parties jouees: %lu\n", st->games_played);
    printf("- Victoires     : %lu\n", st->games_won);

    double win_rate = (st->games_played > 0)
        ? (100.0 * (double)st->games_won / (double)st->games_played)
        : 0.0;
    printf("- Taux de victoire: %.1f%%\n", win_rate);

    double avg_tries = (st->games_played > 0)
        ? ((double)st->total_tries / (double)st->games_played)
        : 0.0;
    printf("- Tentatives moyennes: %.2f\n", avg_tries);

    double avg_time = (st->games_played > 0)
        ? (st->total_time / (double)st->games_played)
        : 0.0;
    printf("- Temps moyen par partie: %.2fs\n\n", avg_time);
}

/* -------------------------
   IA (heuristique type Knuth “light”)
   ------------------------- */

/*
  feedback_between:
  juste un alias “lisible” autour de compute_feedback.
  L’IA compare un code candidat avec un guess et regarde le feedback.
*/
static void feedback_between(const char secret[CODE_LEN],
                             const char guess[CODE_LEN],
                             int *black, int *white) {
    compute_feedback(secret, guess, black, white);
}

/*
  generate_all_codes:
  génère toutes les combinaisons possibles selon la config.
  - si répétitions ON : color_count^4 possibilités
  - sinon : permutations sans répétition
  Attention: tu stockes dans codes[][4], donc faut que le tableau soit assez grand.
*/
static int generate_all_codes(char codes[][CODE_LEN], const GameConfig *cfg) {
    int count = 0;

    if (cfg->allow_repetition) {
        for (int i0 = 0; i0 < cfg->color_count; i0++)
        for (int i1 = 0; i1 < cfg->color_count; i1++)
        for (int i2 = 0; i2 < cfg->color_count; i2++)
        for (int i3 = 0; i3 < cfg->color_count; i3++) {
            codes[count][0] = GLOBAL_COLOR_SET[i0];
            codes[count][1] = GLOBAL_COLOR_SET[i1];
            codes[count][2] = GLOBAL_COLOR_SET[i2];
            codes[count][3] = GLOBAL_COLOR_SET[i3];
            count++;
        }
    } else {
        // Version “sans répétition”: on impose i1!=i0, i2!=i0,i1, etc.
        for (int i0 = 0; i0 < cfg->color_count; i0++)
        for (int i1 = 0; i1 < cfg->color_count; i1++) if (i1 != i0)
        for (int i2 = 0; i2 < cfg->color_count; i2++) if (i2 != i0 && i2 != i1)
        for (int i3 = 0; i3 < cfg->color_count; i3++) if (i3 != i0 && i3 != i1 && i3 != i2) {
            codes[count][0] = GLOBAL_COLOR_SET[i0];
            codes[count][1] = GLOBAL_COLOR_SET[i1];
            codes[count][2] = GLOBAL_COLOR_SET[i2];
            codes[count][3] = GLOBAL_COLOR_SET[i3];
            count++;
        }
    }

    return count;
}

/*
  evaluate_guess:
  idée: pour un guess donné, on regarde la “pire” taille de partition possible.
  - On simule le feedback (b,w) pour chaque secret possible restant.
  - On compte combien tombent dans chaque case (b,w).
  - Le score = max(partition) (pire cas).
  L’IA veut minimiser ce pire cas.
*/
static int evaluate_guess(const char guess[CODE_LEN],
                          char possibles[][CODE_LEN],
                          const bool actif[],
                          int nb_possibles) {
    int worst_partition = 0;
    int counts[25]; // 5 blacks possibles (0..4) * 5 whites possibles (0..4) => 25 cases

    for (int k = 0; k < 25; k++) counts[k] = 0;

    for (int i = 0; i < nb_possibles; i++) {
        if (!actif[i]) continue;

        int b, w;
        feedback_between(possibles[i], guess, &b, &w);

        int idx = b * 5 + w; // mapping (b,w) -> index (simple et rapide)
        counts[idx]++;
    }

    for (int k = 0; k < 25; k++)
        if (counts[k] > worst_partition)
            worst_partition = counts[k];

    return worst_partition;
}

/*
  choose_next_guess:
  brute force: teste tous les codes encore “actifs” comme prochain guess.
  prend celui qui minimise evaluate_guess (donc minimax).
  C’est coûteux, mais vu les tailles (max ~1296 avec répétitions et 6 couleurs) ça reste jouable.
*/
static void choose_next_guess(char guess_out[CODE_LEN],
                              char possibles[][CODE_LEN],
                              bool actif[],
                              int nb_possibles) {
    int best_score = 999999;
    int best_index = -1;

    for (int i = 0; i < nb_possibles; i++) {
        if (!actif[i]) continue;

        int score = evaluate_guess(possibles[i], possibles, actif, nb_possibles);
        if (score < best_score) {
            best_score = score;
            best_index = i;
        }
    }

    // fallback au cas où (normalement best_index devrait exister)
    if (best_index == -1) {
        for (int i = 0; i < nb_possibles; i++) {
            if (actif[i]) { best_index = i; break; }
        }
    }

    if (best_index != -1) memcpy(guess_out, possibles[best_index], CODE_LEN);
    else {
        // plan Z: met tout à la première couleur (ça évite de laisser guess_out non initialisé)
        for (int i = 0; i < CODE_LEN; i++) guess_out[i] = GLOBAL_COLOR_SET[0];
    }
}

/*
  filter_possibilities:
  après un guess + feedback (noirs_attendus, blancs_attendus),
  on élimine tous les codes qui ne donneraient pas le même feedback.
*/
static int filter_possibilities(char possibles[][CODE_LEN],
                                bool actif[],
                                int nb_possibles,
                                const char guess[CODE_LEN],
                                int noirs_attendus,
                                int blancs_attendus) {
    int count = 0;

    for (int i = 0; i < nb_possibles; i++) {
        if (!actif[i]) continue;

        int b, w;
        feedback_between(possibles[i], guess, &b, &w);

        if (b == noirs_attendus && w == blancs_attendus) {
            count++;
        } else {
            actif[i] = false;
        }
    }

    return count;
}

/* -------------------------
   Boucles de jeu (Humain / IA)
   ------------------------- */

/*
  play_human:
  - génère un secret
  - boucle jusqu’à trouver ou dépasser max_tries
  - à chaque tentative: parse + feedback + stockage historique
  - si victoire/défaite: maj stats + save_stats
  Bonus: l’utilisateur peut taper "save" pendant la partie pour sauvegarder.
*/
static void play_human(GameConfig cfg, Stats *st) {
    GameState gs = {0};
    gs.cfg = cfg;
    gs.in_progress = true;

    srand((unsigned int)time(NULL)); // seed du RNG (note: seed aussi dans menu_loop → redondant)

    generate_secret(gs.secret, cfg.color_count, cfg.allow_repetition);

    time_t start_part = time(NULL);

    while (gs.tries < cfg.max_tries) {
        char guess[CODE_LEN];
        bool ok = false;

        if (cfg.timed_mode) {
            ok = timed_get_guess(guess, cfg.color_count, cfg.allow_repetition, cfg.time_per_try_sec);
        } else {
            char line[256];
            if (!read_line(line, sizeof(line))) continue;
            ok = parse_guess(line, guess, cfg.color_count, cfg.allow_repetition);
        }

        if (!ok) {
            // Ici tu dis “entrée invalide ou hors temps” et tu continues sans consommer de tentative
            // (ce qui est un choix de design, pas une obligation)
            continue;
        }

        int black = 0, white = 0;
        compute_feedback(gs.secret, guess, &black, &white);

        memcpy(gs.guesses[gs.tries], guess, CODE_LEN);
        gs.blacks[gs.tries] = black;
        gs.whites[gs.tries] = white;
        gs.tries++;

        if (black == CODE_LEN) {
            time_t end_part = time(NULL);
            double elapsed = difftime(end_part, start_part);

            st->games_played++;
            st->games_won++;
            st->total_tries += gs.tries;
            st->total_time += elapsed;

            save_stats(st, "stats.txt");
            gs.in_progress = false;
            return;
        }

        // Petite commande après chaque tour: save / quit / rien
        char cmd[32];
        if (read_line(cmd, sizeof(cmd))) {
            if (strcmp(cmd, "save") == 0) save_game(&gs, "save.txt");
            else if (strcmp(cmd, "quit") == 0) break;
        }
    }

    // Défaite
    time_t end_part = time(NULL);
    double elapsed = difftime(end_part, start_part);

    st->games_played++;
    st->total_tries += gs.tries;
    st->total_time += elapsed;

    save_stats(st, "stats.txt");
}

/*
  play_ai:
  - génère un secret “caché”
  - génère toutes les possibilités compatibles avec la config
  - garde un tableau actif[] pour éliminer
  - à chaque tour: choisit un guess (minimax), calcule feedback, filtre
*/
static void play_ai(GameConfig cfg, Stats *st) {
    char secret[CODE_LEN];
    generate_secret_ai(secret, cfg.color_count, cfg.allow_repetition);

    char possibles[2000][CODE_LEN]; // 2000 suffit pour 6^4=1296 (répétitions ON)
    bool actif[2000];

    int nb_possibles = generate_all_codes(possibles, &cfg);
    for (int i = 0; i < nb_possibles; i++) actif[i] = true;

    int tries = 0;
    time_t start_part = time(NULL);

    while (tries < cfg.max_tries) {
        char guess[CODE_LEN];
        choose_next_guess(guess, possibles, actif, nb_possibles);

        int black = 0, white = 0;
        compute_feedback(secret, guess, &black, &white);
        tries++;

        if (black == CODE_LEN) {
            time_t end_part = time(NULL);
            double elapsed = difftime(end_part, start_part);

            st->games_played++;
            st->total_tries += tries;
            st->total_time += elapsed;
            save_stats(st, "stats.txt");
            return;
        }

        filter_possibilities(possibles, actif, nb_possibles, guess, black, white);
    }

    // Échec IA
    time_t end_part = time(NULL);
    double elapsed = difftime(end_part, start_part);

    st->games_played++;
    st->total_tries += tries;
    st->total_time += elapsed;
    save_stats(st, "stats.txt");
}

/* -------------------------
   Menu + config utilisateur
   ------------------------- */

/*
  configure_game:
  propose presets ou personnalisation.
  Les helpers ask_int/ask_yes_no bouclent jusqu’à une entrée valide → UX console propre.
*/
static int ask_int(const char *prompt, int minv, int maxv) {
    char buf[128];
    int val;

    while (1) {
        printf("%s [%d..%d]: ", prompt, minv, maxv);
        if (!read_line(buf, sizeof(buf))) continue;
        if (sscanf(buf, "%d", &val) == 1 && val >= minv && val <= maxv) return val;
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
    char line[64];
    if (!read_line(line, sizeof(line))) return;

    int c = atoi(line);

    switch (c) {
        case 1: set_preset_easy(cfg); break;
        case 2: set_preset_intermediate(cfg); break;
        case 3: set_preset_hard(cfg); break;
        case 4: set_preset_expert(cfg); break;
        case 5:
            cfg->color_count = ask_int("Nombre de couleurs", MIN_COLORS, MAX_COLORS);
            cfg->max_tries   = ask_int("Nombre de tentatives", MAX_TRIES_MIN, MAX_TRIES_MAX);
            cfg->allow_repetition = ask_yes_no("Autoriser les repetitions ?");
            cfg->timed_mode = ask_yes_no("Activer le chronometre strict ?");
            cfg->time_per_try_sec = cfg->timed_mode ? ask_int("Temps par tentative (s)", 10, 300) : 0;
            break;
        default:
            break;
    }
}

/*
  menu_loop:
  boucle principale:
  - charge stats au lancement
  - affiche menu
  - appelle le bon mode
*/
static void menu_loop(void) {
    GameConfig cfg;
    set_default_config(&cfg);

    Stats stats;
    load_stats(&stats, "stats.txt");

    srand((unsigned int)time(NULL));

    while (1) {
        char line[64];
        if (!read_line(line, sizeof(line))) continue;

        int choice = atoi(line);

        switch (choice) {
            case 1: play_human(cfg, &stats); break;
            case 2: play_ai(cfg, &stats); break;
            case 3: configure_game(&cfg); break;
            case 4: print_rules(); break;
            case 5: print_stats(&stats); break;
            case 6: resume_game(&stats); break;
            case 0: return;
            default: break;
        }
    }
}

int main(void) {
    menu_loop();
    return 0;
}

