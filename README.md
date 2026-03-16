# Labyrinthe — Division récursive + BFS 

Ce projet génère et résout un labyrinthe en C avec animation SDL2.

## Fonctionnement

Le programme se déroule en **4 phases visuelles** :

1. **Génération animée** — construction du labyrinthe par
   division récursive (murs blancs sur fond sombre)
2. **Exploration BFS animée** — parcours en largeur,
   cellules visitées colorées en bleu
3. **Effacement** — retour au labyrinthe vide
4. **Chemin le plus court** — tracé en jaune doré

| Couleur | Signification |
|---------|---------------|
| 🟢 Vert | Point de départ (coin haut-gauche) |
| 🔴 Rouge | Point d'arrivée (coin bas-droite) |
| 🔵 Bleu | Cellules explorées par BFS |
| 🟡 Jaune | Chemin le plus court |

## Algorithmes

### Génération — Division récursive

Le labyrinthe est généré par **division récursive** :
- On divise la grille par un mur horizontal ou vertical
- On laisse une ouverture aléatoire dans chaque mur
- On répète récursivement dans chaque sous-région
- Implémenté avec une **pile explicite** (pas de récursion
  réelle pour éviter le débordement de pile)

### Résolution — BFS (Breadth-First Search)

La résolution utilise le **parcours en largeur** :
- Garantit le chemin le **plus court** en nombre de cellules
- File d'attente FIFO circulaire implémentée manuellement
- Tableau `prev[]` pour reconstruire le chemin à rebours

```
Complexité BFS : O(n)  où n = rows × cols
```

## Compilation

```bash
# Linux / macOS
gcc -o maze maze.c $(sdl2-config --cflags --libs)

# Windows (MinGW)
gcc -o maze maze.c -lSDL2 -lSDL2main
```

## Dépendances

- SDL2 (`libsdl2-dev` sur Ubuntu/Debian)

```bash
sudo apt install libsdl2-dev
```

## Paramètres modifiables

Dans `main()` :

```c
int rows          = 20;   /* nombre de lignes        */
int cols          = 30;   /* nombre de colonnes      */
int cellSize      = 28;   /* taille d'une cellule px */
int wallThickness = 3;    /* épaisseur des murs px   */
```

## Compétences acquises

- Allocation dynamique de mémoire en C (malloc/free)
- Implémentation d'une file FIFO circulaire
- Algorithme BFS et reconstruction de chemin
- Génération procédurale par division récursive
- Animation temps réel avec SDL2
- Gestion d'une pile explicite en C



## Auteur

Tarash Pavlo
