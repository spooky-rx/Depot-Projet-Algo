/*
 * Mastermind (jeu de base) - Console en C
 *
 * Règles:
 * - Code secret de 4 couleurs, choisies sans répétition
 * - 10 tentatives pour deviner
 * - Feedback:
 *     Noir  (●): bonne couleur, bien placée
 *     Blanc (○): bonne couleur, mal placée
 *
 * Entrée utilisateur:
 * - Saisir une proposition de 4 lettres parmi: R G B Y O P (Rouge, Vert, Bleu, Jaune, Orange, Violet)
 * - Sans répétition (ex: RGBY ou R G B Y)
 * - Insensible aux espaces, insensible à la casse (r == R)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>

#define CODE_LEN 4          // Longueur du code secret
#define MAX_TRIES 10        // Nombre de tentatives autorisées
#define COLOR_COUNT 6       // Nombre de couleurs disponibles

// Ensemble des couleurs disponibles (lettres) et leurs noms lisibles
static const char COLOR_SET[COLOR_COUNT] = { 'R', 'G', 'B', 'Y', 'O', 'P' };
static const char *COLOR_NAMES[COLOR_COUNT] = { "Rouge", "Vert", "Bleu", "Jaune", "Orange", "Violet" };

/* =========================
   Utilitaires d'affichage
   ========================= */

/**
 * Affiche la palette disponible sous forme "Lettre: Nom".
 */
static void print_palette(void) {
    printf("Couleurs disponibles (sans répétition):\n");
    for (int i = 0; i < COLOR_COUNT; i++) {
        printf("  %c = %s\n", COLOR_SET[i], COLOR_NAMES[i]);
    }
    printf("Saisissez 4 lettres (ex: RGBY ou R G B Y).\n");
}

/**
 * Affiche une proposition (tableau de 4 lettres).
 */
static void print_code(const char code[CODE_LEN]) {
    for (int i = 0; i < CODE_LEN; i++) {
        printf("%c", code[i]);
    }
}

/* =========================
   Validation et parsing
   ========================= */

/**
 * Vérifie si une lettre correspond à une couleur valide.
 * Retourne true si valide, false sinon.
 */
static bool is_valid_color_char(char c) {
    c = (char)toupper((unsigned char)c);
    for (int i = 0; i < COLOR_COUNT; i++) {
        if (COLOR_SET[i] == c) return true;
    }
    return false;
}

/**
 * Vérifie l'absence de répétitions dans un code de longueur CODE_LEN.
 */
static bool has_no_repetition(const char code[CODE_LEN]) {
    for (int i = 0; i < CODE_LEN; i++) {
        for (int j = i + 1; j < CODE_LEN; j++) {
            if (code[i] == code[j]) return false;
        }
    }
    return true;
}

/**
 * Lit une ligne depuis stdin dans buffer (taille buflen).
 * Retourne true si une ligne a été lue, false sinon.
 */
static bool read_line(char *buffer, size_t buflen) {
    if (fgets(buffer, (int)buflen, stdin) == NULL) {
        return false;
    }
    // Retire le '\n' éventuel
    size_t n = strlen(buffer);
    if (n > 0 && buffer[n - 1] == '\n') buffer[n - 1] = '\0';
    return true;
}

/**
 * Parse une entrée utilisateur en un code de 4 lettres.
 * Accepte les formats avec ou sans espaces, et est insensible à la casse.
 * Exemple d'entrées: "RGBY", "r g b y", "R, G, B, Y"
 *
 * Retour:
 * - true si parsing réussi et code valide
 * - false sinon
 */
static bool parse_guess(const char *line, char out_code[CODE_LEN]) {
    // Collecte des lettres valides présentes dans la ligne
    int count = 0;
    for (const char *p = line; *p != '\0'; ++p) {
        char c = *p;
        if (isalpha((unsigned char)c)) {
            c = (char)toupper((unsigned char)c);
            if (is_valid_color_char(c)) {
                if (count < CODE_LEN) {
                    out_code[count++] = c;
                } else {
                    // Trop de lettres valides
                    return false;
                }
            } else {
                // Lettre alphabétique mais pas une couleur autorisée
                return false;
            }
        }
        // Les autres caractères (espaces, virgules, etc.) sont ignorés
    }

    if (count != CODE_LEN) {
        // Pas assez de lettres valides
        return false;
    }

    // Vérifie l'absence de répétitions (contrainte du jeu de base)
    if (!has_no_repetition(out_code)) {
        return false;
    }

    return true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* =========================
   Génération du code secret
   ========================= */

/**
 * Génère un code secret de 4 couleurs, sans répétition.
 * Utilise une permutation partielle de COLOR_SET.
 */
static void generate_secret(char secret[CODE_LEN]) {
    // Copie mutable de la palette
    char pool[COLOR_COUNT];
    for (int i = 0; i < COLOR_COUNT; i++) pool[i] = COLOR_SET[i];

    // Mélange Fisher-Yates
    for (int i = COLOR_COUNT - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        char tmp = pool[i];
        pool[i] = pool[j];
        pool[j] = tmp;
    }

    // Prend les 4 premiers pour le code secret
    for (int k = 0; k < CODE_LEN; k++) {
        secret[k] = pool[k];
    }
}

/* =========================
   Calcul du feedback
   ========================= */

/**
 * Calcule le feedback (noirs/blancs) entre secret et guess.
 * - Noir: position et couleur exactes
 * - Blanc: bonne couleur mais mauvaise position
 *
 * Stratégie:
 * 1) Compte les noirs (positions exactes)
 * 2) Pour le reste, compte les occurrences de couleurs dans secret et guess,
 *    puis somme du minimum des occurrences pour obtenir le total correspondances,
 *    dont on soustrait les noirs pour avoir les blancs.
 */
static void compute_feedback(const char secret[CODE_LEN],
                             const char guess[CODE_LEN],
                             int *black, int *white) {
    *black = 0;
    *white = 0;

    // Étape 1: noirs
    bool secret_used[CODE_LEN] = { false };
    bool guess_used[CODE_LEN]  = { false };
    for (int i = 0; i < CODE_LEN; i++) {
        if (secret[i] == guess[i]) {
            (*black)++;
            secret_used[i] = true;
            guess_used[i]  = true;
        }
    }

    // Étape 2: blancs via comptage des lettres restantes
    int secret_count[256] = {0};
    int guess_count[256]  = {0};

    for (int i = 0; i < CODE_LEN; i++) {
        if (!secret_used[i]) {
            secret_count[(unsigned char)secret[i]]++;
        }
        if (!guess_used[i]) {
            guess_count[(unsigned char)guess[i]]++;
        }
    }

    int matches = 0;
    for (int c = 0; c < 256; c++) {
        if (secret_count[c] > 0 || guess_count[c] > 0) {
            matches += (secret_count[c] < guess_count[c]) ? secret_count[c] : guess_count[c];
        }
    }

    *white = matches;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* =========================
   Boucle principale du jeu
   ========================= */

int main(void) {
    // Initialise le générateur aléatoire (basé sur l'heure)
    srand((unsigned int)time(NULL));

    printf("=== Mastermind (Jeu de base) ===\n\n");
    print_palette();
    printf("\nObjectif: devinez le code secret en %d tentatives.\n", MAX_TRIES);
    printf("Feedback: ● = noir (bien placé), ○ = blanc (bonne couleur, mauvaise position)\n\n");

    // Génère le code secret
    char secret[CODE_LEN];
    generate_secret(secret);

    // Historique minimal: garder les essais et feedback
    char history_guess[MAX_TRIES][CODE_LEN];
    int history_black[MAX_TRIES];
    int history_white[MAX_TRIES];
    int tries = 0;

    // Boucle des tentatives
    while (tries < MAX_TRIES) {
        printf("Tentative %d/%d - Entrez votre proposition: ", tries + 1, MAX_TRIES);
        char line[256];
        if (!read_line(line, sizeof(line))) {
            printf("\nErreur de lecture. Veuillez réessayer.\n");
            continue;
        }

        char guess[CODE_LEN];
        if (!parse_guess(line, guess)) {
            printf("Entrée invalide. Rappel: 4 lettres parmi R G B Y O P, sans répétition (ex: RGBY).\n");
            continue;
        }

        int black = 0, white = 0;
        compute_feedback(secret, guess, &black, &white);

        // Stocke dans l'historique
        for (int i = 0; i < CODE_LEN; i++) history_guess[tries][i] = guess[i];
        history_black[tries] = black;
        history_white[tries] = white;

        // Affiche le feedback pour cette tentative
        printf("Vous avez proposé: ");
        print_code(guess);
        printf("  => ●: %d, ○: %d\n", black, white);

        tries++;

        // Condition de victoire
        if (black == CODE_LEN) {
            printf("\nBravo ! Vous avez deviné le code en %d tentative%s.\n",
                   tries, (tries > 1 ? "s" : ""));
            printf("Code secret: ");
            print_code(secret);
            printf("\n");
            break;
        }

        // Option: afficher un petit tableau récapitulatif (historique compact)
        printf("Historique des essais:\n");
        for (int i = 0; i < tries; i++) {
            printf("  %2d) ", i + 1);
            print_code(history_guess[i]);
            printf("  ●: %d, ○: %d\n", history_black[i], history_white[i]);
        }
        printf("\n");
    }

    // Défaite si toutes les tentatives utilisées sans trouver
    if (tries == MAX_TRIES) {
        printf("Dommage ! Vous n'avez pas trouvé le code.\n");
        printf("Le code secret était: ");
        print_code(secret);
        printf("\n");
    }

    printf("\nMerci d'avoir joué !\n");
    return 0;
}

