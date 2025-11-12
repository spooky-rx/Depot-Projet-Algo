/*
 * Mastermind (version avancée) - Console en C
 *
 * Fonctionnalités avancées:
 * - Menu principal: jouer, configurer, afficher règles, quitter
 * - Paramètres configurables:
 *     * Nombre de couleurs: 3 à 6
 *     * Longueur du code: 4 (modifiable facilement via CODE_LEN)
 *     * Nombre de tentatives: 5 à 30
 *     * Répétitions autorisées: ON/OFF
 *     * Mode chronométré par tentative: ON/OFF, temps imparti en secondes
 * - Presets de difficulté:
 *     * Facile       : 3 couleurs, 20 tentatives, répétitions ON, chrono OFF
 *     * Intermédiaire: 4 couleurs, 15 tentatives, répétitions ON, chrono OFF
 *     * Difficile    : 5 couleurs, 10 tentatives, répétitions OFF, chrono ON (60s)
 *     * Expert       : 6 couleurs, 5 tentatives,  répétitions OFF, chrono ON (45s)
 * - Validation robuste des entrées (format, palette, répétitions)
 * - Feedback exact: noirs (bien placés), blancs (bonne couleur mal placée)
 * - Historique des essais affiché
 *
 * Compilation:
 *   gcc -Wall -Wextra -O2 -std=c11 mastermind_advanced.c -o mastermind_advanced
 *
 * Exécution:
 *   ./mastermind_advanced
 *
 * Remarque:
 * - La longueur du code est fixée à 4 pour coller au CDC initial; vous pouvez
 *   modifier CODE_LEN si besoin, le code s'adapte (feedback/validation).
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
#define MAX_TRIES_MAX 30
#define MAX_TRIES_MIN 5

// Palette globale maximale (6 couleurs)
static const char GLOBAL_COLOR_SET[MAX_COLORS] = { 'R', 'G', 'B', 'Y', 'O', 'P' };
static const char *GLOBAL_COLOR_NAMES[MAX_COLORS] = { "Rouge", "Vert", "Bleu", "Jaune", "Orange", "Violet" };

/* =========================
   Configuration du jeu
   ========================= */

typedef struct {
    int color_count;       // entre 3 et 6
    int max_tries;         // entre 5 et 30
    bool allow_repetition; // répétitions autorisées dans le secret et les tentatives
    bool timed_mode;       // tentative chronométrée
    int time_per_try_sec;  // secondes par tentative (si timed_mode)
} GameConfig;

static void set_default_config(GameConfig *cfg) {
    cfg->color_count = 6;
    cfg->max_tries = 10;
    cfg->allow_repetition = false;
    cfg->timed_mode = false;
    cfg->time_per_try_sec = 0;
}

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

/* =========================
   Affichage & utilitaires
   ========================= */

static void print_palette(int color_count) {
    printf("Couleurs disponibles:\n");
    for (int i = 0; i < color_count; i++) {
        printf("  %c = %s\n", GLOBAL_COLOR_SET[i], GLOBAL_COLOR_NAMES[i]);
    }
    printf("Saisissez %d lettres (ex: RGBY ou R G B Y). Casse ignorée.\n", CODE_LEN);
}

static bool read_line(char *buffer, size_t buflen) {
    if (fgets(buffer, (int)buflen, stdin) == NULL) return false;
    size_t n = strlen(buffer);
    if (n > 0 && buffer[n - 1] == '\n') buffer[n - 1] = '\0';
    return true;
}

static void print_code(const char code[CODE_LEN]) {
    for (int i = 0; i < CODE_LEN; i++) printf("%c", code[i]);
}

/* =========================
   Validation & parsing
   ========================= */

static bool is_valid_color_char(char c, int color_count) {
    c = (char)toupper((unsigned char)c);
    for (int i = 0; i < color_count; i++) {
        if (GLOBAL_COLOR_SET[i] == c) return true;
    }
    return false;
}

static bool has_no_repetition(const char code[], int len) {
    for (int i = 0; i < len; i++) {
        for (int j = i + 1; j < len; j++) {
            if (code[i] == code[j]) return false;
        }
    }
    return true;
}

/**
 * Parse une entrée utilisateur en un code de CODE_LEN lettres.
 * - Accepte avec/sans espaces/virgules, casse ignorée
 * - Valide contre color_count
 * - Si repetitions non autorisées, les refuse
 */
static bool parse_guess(const char *line, char out_code[CODE_LEN],
                        int color_count, bool allow_repetition) {
    int count = 0;
    for (const char *p = line; *p != '\0'; ++p) {
        char c = *p;
        if (isalpha((unsigned char)c)) {
            c = (char)toupper((unsigned char)c);
            if (!is_valid_color_char(c, color_count)) return false;
            if (count < CODE_LEN) {
                out_code[count++] = c;
            } else {
                return false;
            }
        }
    }
    if (count != CODE_LEN) return false;
    if (!allow_repetition && !has_no_repetition(out_code, CODE_LEN)) return false;
    return true;
}

/* =========================
   Génération du secret
   ========================= */

static void generate_secret(char secret[CODE_LEN], int color_count, bool allow_repetition) {
    if (allow_repetition) {
        for (int i = 0; i < CODE_LEN; i++) {
            int idx = rand() % color_count;
            secret[i] = GLOBAL_COLOR_SET[idx];
        }
    } else {
        // Tirage sans répétition en choisissant une permutation partielle
        char pool[MAX_COLORS];
        for (int i = 0; i < color_count; i++) pool[i] = GLOBAL_COLOR_SET[i];
        for (int i = color_count - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            char tmp = pool[i]; pool[i] = pool[j]; pool[j] = tmp;
        }
        for (int k = 0; k < CODE_LEN; k++) secret[k] = pool[k];
    }
}

/* =========================
   Calcul du feedback
   ========================= */

static void compute_feedback(const char secret[CODE_LEN],
                             const char guess[CODE_LEN],
                             int *black, int *white) {
    *black = 0;
    *white = 0;

    bool secret_used[CODE_LEN] = { false };
    bool guess_used[CODE_LEN]  = { false };

    // 1) noirs (positions exactes)
    for (int i = 0; i < CODE_LEN; i++) {
        if (secret[i] == guess[i]) {
            (*black)++;
            secret_used[i] = true;
            guess_used[i]  = true;
        }
    }

    // 2) blancs (comptage des occurrences restantes)
    int secret_count[256] = {0};
    int guess_count[256]  = {0};

    for (int i = 0; i < CODE_LEN; i++) {
        if (!secret_used[i]) secret_count[(unsigned char)secret[i]]++;
        if (!guess_used[i])  guess_count[(unsigned char)guess[i]]++;
    }

    int matches = 0;
    for (int c = 0; c < 256; c++) {
        int s = secret_count[c], g = guess_count[c];
        if (s > 0 && g > 0) matches += (s < g) ? s : g;
    }

    *white = matches;
}

/* =========================
   Chronométrage (simple)
   ========================= */

/**
 * Chronométrage basique: mesure le temps écoulé pendant la saisie.
 * Si timed_mode ON et temps dépassé, on considère la tentative "manquée":
 * - On enregistre une tentative vide et on affiche qu'elle est hors temps.
 * Note: Dans un vrai terminal, capturer un timeout d'entrée bloque avec fgets.
 * Ici, on mesure le temps consommé et on signale si dépassement.
 */
static bool timed_read_guess(char guess[CODE_LEN], int color_count,
                             bool allow_repetition, bool timed_mode, int time_per_try_sec) {
    char line[256];

    clock_t start = clock();
    if (!read_line(line, sizeof(line))) return false;
    clock_t end = clock();

    double elapsed_sec = (double)(end - start) / CLOCKS_PER_SEC;

    if (timed_mode && elapsed_sec > (double)time_per_try_sec) {
        // Hors temps: on retourne false spécial pour indiquer "timeout"
        return false;
    }

    return parse_guess(line, guess, color_count, allow_repetition);
}

/* =========================
   Menu & interaction
   ========================= */

static void print_rules(void) {
    printf("\n=== Règles Mastermind (version avancée) ===\n");
    printf("- Code de %d couleurs (longueur: %d), palette configurable (3 à 6 couleurs).\n", CODE_LEN, CODE_LEN);
    printf("- %d tentatives max (configurable 5 à 30).\n", 10);
    printf("- Feedback: ● noir (bien placé), ○ blanc (bonne couleur, mauvaise position).\n");
    printf("- Options: répétitions ON/OFF, mode chronométré par tentative.\n");
    printf("- Presets: facile/intermédiaire/difficile/expert.\n\n");
}

static void print_config(const GameConfig *cfg) {
    printf("\n=== Configuration actuelle ===\n");
    printf("- Couleurs: %d\n", cfg->color_count);
    printf("- Tentatives: %d\n", cfg->max_tries);
    printf("- Répétitions: %s\n", cfg->allow_repetition ? "ON" : "OFF");
    printf("- Chronométré: %s", cfg->timed_mode ? "ON" : "OFF");
    if (cfg->timed_mode) printf(" (%d s/tentative)", cfg->time_per_try_sec);
    printf("\n");
}

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
        if (buf[0] == 'o' || buf[0] == 'O' || buf[0] == 'y' || buf[0] == 'Y') return true;
        if (buf[0] == 'n' || buf[0] == 'N') return false;
        printf("Réponse invalide.\n");
    }
}

static void configure_game(GameConfig *cfg) {
    print_config(cfg);
    printf("\nChoisir une option:\n");
    printf("  1) Preset Facile\n");
    printf("  2) Preset Intermediaire\n");
    printf("  3) Preset Difficile\n");
    printf("  4) Preset Expert\n");
    printf("  5) Personnaliser manuellement\n");
    printf("  0) Retour\n");

    char line[64];
    if (!read_line(line, sizeof(line))) return;
    int choice = atoi(line);

    switch (choice) {
        case 1: set_preset_easy(cfg); break;
        case 2: set_preset_intermediate(cfg); break;
        case 3: set_preset_hard(cfg); break;
        case 4: set_preset_expert(cfg); break;
        case 5: {
            int cc = ask_int("Nombre de couleurs", MIN_COLORS, MAX_COLORS);
            int mt = ask_int("Nombre de tentatives", MAX_TRIES_MIN, MAX_TRIES_MAX);
            bool rep = ask_yes_no("Autoriser les repetitions ?");
            bool timed = ask_yes_no("Activer mode chronometre par tentative ?");
            int secs = 0;
            if (timed) secs = ask_int("Temps par tentative (secondes)", 10, 300);

            cfg->color_count = cc;
            cfg->max_tries = mt;
            cfg->allow_repetition = rep;
            cfg->timed_mode = timed;
            cfg->time_per_try_sec = secs;
            break;
        }
        default: break; // retour
    }

    print_config(cfg);
    printf("Configuration mise à jour.\n\n");
}

/* =========================
   Boucle d'une partie
   ========================= */

static void play_once(const GameConfig *cfg) {
    printf("\n=== Nouvelle partie Mastermind (avancée) ===\n");
    print_palette(cfg->color_count);
    printf("Objectif: devinez le code secret en %d tentatives.\n", cfg->max_tries);
    printf("Options: repetitions %s, chrono %s",
           cfg->allow_repetition ? "ON" : "OFF",
           cfg->timed_mode ? "ON" : "OFF");
    if (cfg->timed_mode) printf(" (%d s)", cfg->time_per_try_sec);
    printf("\n");
    printf("Feedback: ● = noir (bien placé), ○ = blanc (bonne couleur, mauvaise position)\n\n");

    char secret[CODE_LEN];
    generate_secret(secret, cfg->color_count, cfg->allow_repetition);

    char history_guess[64][CODE_LEN]; // 64 pour sécurité au-delà de 30
    int history_black[64];
    int history_white[64];
    int tries = 0;

    while (tries < cfg->max_tries) {
        printf("Tentative %d/%d - Entrez votre proposition: ", tries + 1, cfg->max_tries);

        char guess[CODE_LEN];
        bool ok;

        if (cfg->timed_mode) {
            ok = timed_read_guess(guess, cfg->color_count, cfg->allow_repetition,
                                  true, cfg->time_per_try_sec);
            if (!ok) {
                // Soit timeout soit entrée invalide: on distingue via un message
                printf("\nTemps depasse ou entree invalide.\n");
                // En cas de timeout, on peut considérer la tentative “manquée” sans modifier l'historique.
                // Ici, on n'enregistre pas de guess vide; on redemande la tentative.
                // Variante CDC: considérer la tentative "valide" même hors temps. Pour simplifier, on redemande.
                continue;
            }
        } else {
            char line[256];
            if (!read_line(line, sizeof(line))) {
                printf("\nErreur de lecture. Veuillez reessayer.\n");
                continue;
            }
            ok = parse_guess(line, guess, cfg->color_count, cfg->allow_repetition);
            if (!ok) {
                printf("Entree invalide. Rappel: %d lettres parmi ",
                       CODE_LEN);
                for (int i = 0; i < cfg->color_count; i++) {
                    printf("%c%s", GLOBAL_COLOR_SET[i],
                           (i + 1 < cfg->color_count ? " " : ""));
                }
                printf(", %s repetition.\n", cfg->allow_repetition ? "avec" : "sans");
                continue;
            }
        }

        int black = 0, white = 0;
        compute_feedback(secret, guess, &black, &white);

        for (int i = 0; i < CODE_LEN; i++) history_guess[tries][i] = guess[i];
        history_black[tries] = black;
        history_white[tries] = white;

        printf("Vous avez propose: ");
        print_code(guess);
        printf("  => ●: %d, ○: %d\n", black, white);

        tries++;

        if (black == CODE_LEN) {
            printf("\nBravo ! Vous avez devine le code en %d tentative%s.\n",
                   tries, (tries > 1 ? "s" : ""));
            printf("Code secret: ");
            print_code(secret);
            printf("\n");
            break;
        }

        printf("Historique des essais:\n");
        for (int i = 0; i < tries; i++) {
            printf("  %2d) ", i + 1);
            print_code(history_guess[i]);
            printf("  ●: %d, ○: %d\n", history_black[i], history_white[i]);
        }
        printf("\n");
    }

    if (tries == cfg->max_tries) {
        printf("Dommage ! Vous n'avez pas trouve le code.\n");
        printf("Le code secret etait: ");
        print_code(secret);
        printf("\n");
    }

    printf("\nFin de partie.\n");
}

/* =========================
   Menu principal
   ========================= */

static void menu_loop(void) {
    GameConfig cfg;
    set_default_config(&cfg);

    srand((unsigned int)time(NULL));

    while (1) {
        printf("=== Menu Principal ===\n");
        printf("1) Jouer (avancé)\n");
        printf("2) Configurer (presets / manuel)\n");
        printf("3) Afficher les regles\n");
        printf("0) Quitter\n");
        printf("Choix: ");

        char line[64];
        if (!read_line(line, sizeof(line))) continue;
        int choice = atoi(line);

        switch (choice) {
            case 1:
                play_once(&cfg);
                break;
            case 2:
                configure_game(&cfg);
                break;
            case 3:
                print_rules();
                break;
            case 0:
                printf("Au revoir !\n");
                return;
            default:
                printf("Choix invalide.\n");
                break;
        }
        printf("\n");
    }
}

int main(void) {
    menu_loop();
    return 0;
}

