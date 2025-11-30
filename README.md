#  Mastermind (Jeu de base) en C

##  Objectif du projet
Développer une version console du jeu **Mastermind** en langage C, en respectant les règles classiques du jeu et les contraintes de programmation (modularité, validation des entrées, feedback clair, documentation).

Ce projet est réalisé en groupe et doit être organisé en plusieurs fichiers (`.c` et `.h`) pour favoriser la lisibilité et la collaboration.

---

##  Règles du jeu
- Le **code secret** est composé de **4 couleurs distinctes** choisies parmi :
  - `R` = Rouge  
  - `G` = Vert  
  - `B` = Bleu  
  - `Y` = Jaune  
  - `O` = Orange  
  - `P` = Violet  
- Le joueur dispose de **10 tentatives maximum** pour deviner le code.
- À chaque tentative, le programme fournit un **feedback** :
  -  **Noir (●)** : bonne couleur, bien placée.  
  -  **Blanc (○)** : bonne couleur, mais mal placée.  
- Le joueur gagne s’il trouve le code avant la fin des tentatives.  
- Sinon, le code secret est révélé à la fin.

---

##  Fonctionnalités attendues
1. **Entrée utilisateur**  
   - Saisie d’une proposition de 4 lettres (ex: `RGBY` ou `R G B Y`).  
   - Insensible à la casse (`r == R`).  
   - Espaces et séparateurs ignorés.  
   - Pas de répétition autorisée.

2. **Validation**  
   - Refuser les entrées invalides (mauvaise lettre, trop ou pas assez de couleurs, répétitions).  
   - Afficher un message d’erreur et redemander une saisie correcte.

3. **Feedback**  
   - Calculer et afficher le nombre de noirs et de blancs pour chaque tentative.  
   - Conserver un **historique des essais** avec feedback.

4. **Boucle de jeu**  
   - Générer un code secret aléatoire sans répétition.  
   - Répéter la saisie et le feedback jusqu’à victoire ou épuisement des tentatives.  
   - Afficher un message de victoire ou de défaite.

5. **Organisation du code**  
   - Séparer en plusieurs fichiers :
     - `colors.*` : gestion des couleurs et affichage.  
     - `utils.*` : fonctions utilitaires (lecture, vérification).  
     - `parse.*` : validation et parsing des entrées.  
     - `feedback.*` : calcul du feedback.  
     - `game.*` : génération du code secret et boucle principale.  
     - `main.c` : point d’entrée du programme.  
   - Utiliser des **fichiers d’en-tête (.h)** pour déclarer les fonctions.

---

##  Évaluation
Le projet sera évalué sur :
- Respect des règles du jeu et du cahier des charges.  
- Modularité et lisibilité du code.  
- Gestion correcte des entrées utilisateur.  
- Feedback fiable et logique.  
- Documentation claire (README + commentaires dans le code).  
- Travail collaboratif et organisation sur GitHub.  
- Bonus possibles : IA, sauvegarde, interface graphique, statistiques.

---

##  Versions complètes
- `versions/mastermind_base.c` : Jeu de base en un seul fichier
- `versions/mastermind_advanced.c` : Jeu avancé en un seul fichier

