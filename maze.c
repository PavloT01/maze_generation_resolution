#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

/* ================================================================
 * Labyrinthe — Génération par division récursive + BFS animé
 * Auteur : PavloT01
 *
 * Phases :
 *   1. Génération animée (division récursive, murs blancs sur fond noir)
 *   2. Exploration BFS animée (cellules visitées en bleu)
 *   3. Effacement de l'exploration (retour au labyrinthe vide)
 *   4. Tracé du chemin le plus court (cellules en jaune)
 * ================================================================ */

#define UP    0
#define RIGHT 1
#define DOWN  2
#define LEFT  3

/* ----------------------------------------------------------------
 * Structures
 * ---------------------------------------------------------------- */

typedef struct {
    int x, y;
    bool walls[4]; /* UP, RIGHT, DOWN, LEFT */
} Cell;

typedef struct {
    int rows, cols;
    Cell **cells;
} Grid;

typedef struct {
    int xStart, yStart, xEnd, yEnd;
} DivideStep;

/* File FIFO pour BFS — tableau circulaire */
typedef struct {
    int *data;
    int head, tail, cap;
} Queue;

/* ----------------------------------------------------------------
 * File d'attente (BFS)
 * ---------------------------------------------------------------- */

Queue queueNew(int cap) {
    Queue q;
    q.cap  = cap;
    q.data = malloc(cap * sizeof(int));
    q.head = 0;
    q.tail = 0;
    return q;
}

void queueFree(Queue *q) {
    free(q->data);
    q->data = NULL;
}

bool queueEmpty(Queue *q) { return q->head == q->tail; }

void queuePush(Queue *q, int v) {
    q->data[q->tail] = v;
    q->tail = (q->tail + 1) % q->cap;
}

int queuePop(Queue *q) {
    int v = q->data[q->head];
    q->head = (q->head + 1) % q->cap;
    return v;
}

/* ----------------------------------------------------------------
 * Grille
 * ---------------------------------------------------------------- */

Grid newGrid(int cols, int rows) {
    Grid g;
    g.rows  = rows;
    g.cols  = cols;
    g.cells = malloc(rows * sizeof(Cell *));
    for (int i = 0; i < rows; i++) {
        g.cells[i] = malloc(cols * sizeof(Cell));
        for (int j = 0; j < cols; j++) {
            g.cells[i][j].x = j;
            g.cells[i][j].y = i;
            for (int k = 0; k < 4; k++)
                g.cells[i][j].walls[k] = false;
        }
    }
    /* Bordures extérieures */
    for (int i = 0; i < rows; i++) {
        g.cells[i][0].walls[LEFT]       = true;
        g.cells[i][cols-1].walls[RIGHT] = true;
    }
    for (int j = 0; j < cols; j++) {
        g.cells[0][j].walls[UP]         = true;
        g.cells[rows-1][j].walls[DOWN]  = true;
    }
    return g;
}

void freeGrid(Grid *g) {
    for (int i = 0; i < g->rows; i++) free(g->cells[i]);
    free(g->cells);
    g->cells = NULL;
}

int rand_int(int a, int b) { return a + rand() % (b - a + 1); }

void setWall(Grid *g, int x, int y, int dir, bool val) {
    g->cells[y][x].walls[dir] = val;
    int nx = x, ny = y, od = 0;
    switch (dir) {
        case UP:    ny = y - 1; od = DOWN;  break;
        case DOWN:  ny = y + 1; od = UP;    break;
        case LEFT:  nx = x - 1; od = RIGHT; break;
        case RIGHT: nx = x + 1; od = LEFT;  break;
    }
    if (nx >= 0 && nx < g->cols && ny >= 0 && ny < g->rows)
        g->cells[ny][nx].walls[od] = val;
}

/* ----------------------------------------------------------------
 * Rendu
 * ---------------------------------------------------------------- */

void drawMaze(SDL_Renderer *ren, Grid *g, int cs, int wt,
              int *overlay,       /* couleur par cellule : -1 = rien */
              int startIdx, int endIdx)
{
    /* Fond */
    SDL_SetRenderDrawColor(ren, 18, 18, 24, 255);
    SDL_RenderClear(ren);

    for (int y = 0; y < g->rows; y++) {
        for (int x = 0; x < g->cols; x++) {
            int idx = y * g->cols + x;
            int px  = x * cs;
            int py  = y * cs;

            /* Couleur de remplissage de la cellule */
            if (overlay != NULL && overlay[idx] >= 0) {
                int col = overlay[idx];
                int r = (col >> 16) & 0xFF;
                int gv = (col >> 8) & 0xFF;
                int b  = col & 0xFF;
                SDL_SetRenderDrawColor(ren, r, gv, b, 200);
                SDL_RenderFillRect(ren, &(SDL_Rect){px + wt, py + wt,
                                                    cs - wt, cs - wt});
            }

            /* Entrée (vert) et sortie (rouge) */
            if (idx == startIdx) {
                SDL_SetRenderDrawColor(ren, 60, 220, 100, 255);
                SDL_RenderFillRect(ren, &(SDL_Rect){px + wt, py + wt,
                                                    cs - wt, cs - wt});
            }
            if (idx == endIdx) {
                SDL_SetRenderDrawColor(ren, 240, 80, 80, 255);
                SDL_RenderFillRect(ren, &(SDL_Rect){px + wt, py + wt,
                                                    cs - wt, cs - wt});
            }

            /* Murs */
            Cell c = g->cells[y][x];
            SDL_SetRenderDrawColor(ren, 220, 220, 230, 255);
            if (c.walls[UP])
                SDL_RenderFillRect(ren, &(SDL_Rect){px, py, cs, wt});
            if (c.walls[RIGHT])
                SDL_RenderFillRect(ren, &(SDL_Rect){px + cs - wt, py, wt, cs});
            if (c.walls[DOWN])
                SDL_RenderFillRect(ren, &(SDL_Rect){px, py + cs - wt, cs, wt});
            if (c.walls[LEFT])
                SDL_RenderFillRect(ren, &(SDL_Rect){px, py, wt, cs});
        }
    }
    SDL_RenderPresent(ren);
}

/* ----------------------------------------------------------------
 * Phase 1 — Génération par division récursive (animée)
 * ---------------------------------------------------------------- */

void generateMaze(SDL_Renderer *ren, Grid *g, int cs, int wt,
                  int startIdx, int endIdx)
{
    DivideStep stack[4096];
    int sp = 0;
    stack[sp++] = (DivideStep){0, 0, g->cols - 1, g->rows - 1};

    while (sp > 0) {
        DivideStep step = stack[--sp];
        int w = step.xEnd - step.xStart + 1;
        int h = step.yEnd - step.yStart + 1;
        if (w <= 1 || h <= 1) continue;

        int orient;
        if (w > h)      orient = 1; /* vertical */
        else if (h > w) orient = 0; /* horizontal */
        else            orient = rand_int(0, 1);

        if (orient == 1) {
            int wallX = rand_int(step.xStart + 1, step.xEnd);
            int gapY  = rand_int(step.yStart, step.yEnd);
            for (int y = step.yStart; y <= step.yEnd; y++)
                if (y != gapY)
                    setWall(g, wallX - 1, y, RIGHT, true);
            stack[sp++] = (DivideStep){step.xStart, step.yStart, wallX-1, step.yEnd};
            stack[sp++] = (DivideStep){wallX, step.yStart, step.xEnd, step.yEnd};
        } else {
            int wallY = rand_int(step.yStart + 1, step.yEnd);
            int gapX  = rand_int(step.xStart, step.xEnd);
            for (int x = step.xStart; x <= step.xEnd; x++)
                if (x != gapX)
                    setWall(g, x, wallY - 1, DOWN, true);
            stack[sp++] = (DivideStep){step.xStart, step.yStart, step.xEnd, wallY-1};
            stack[sp++] = (DivideStep){step.xStart, wallY, step.xEnd, step.yEnd};
        }

        drawMaze(ren, g, cs, wt, NULL, startIdx, endIdx);
        SDL_Delay(60);

        SDL_Event e;
        SDL_PollEvent(&e);
    }
}

/* ----------------------------------------------------------------
 * Phase 2 & 4 — BFS + animation exploration + chemin le plus court
 * ---------------------------------------------------------------- */

void solveMaze(SDL_Renderer *ren, Grid *g, int cs, int wt,
               int startIdx, int endIdx)
{
    int n       = g->rows * g->cols;
    int *prev   = malloc(n * sizeof(int));
    bool *vis   = malloc(n * sizeof(bool));
    int *overlay = malloc(n * sizeof(int));

    for (int i = 0; i < n; i++) {
        prev[i]    = -1;
        vis[i]     = false;
        overlay[i] = -1;
    }

    Queue q = queueNew(n + 4);
    vis[startIdx] = true;
    queuePush(&q, startIdx);

    /* --- Phase 2 : exploration BFS animée (bleu) --- */
    while (!queueEmpty(&q)) {
        int cur = queuePop(&q);
        int cx  = cur % g->cols;
        int cy  = cur / g->cols;

        overlay[cur] = 0x1a3a6e; /* bleu foncé */

        /* Voisins dans les 4 directions si pas de mur */
        int dx[4] = {0, 1, 0, -1};
        int dy[4] = {-1, 0, 1, 0};

        for (int d = 0; d < 4; d++) {
            if (g->cells[cy][cx].walls[d]) continue;
            int nx  = cx + dx[d];
            int ny  = cy + dy[d];
            int nid = ny * g->cols + nx;
            if (nx < 0 || nx >= g->cols || ny < 0 || ny >= g->rows) continue;
            if (vis[nid]) continue;
            vis[nid]  = true;
            prev[nid] = cur;
            queuePush(&q, nid);
        }

        drawMaze(ren, g, cs, wt, overlay, startIdx, endIdx);
        SDL_Delay(18);

        SDL_Event e;
        SDL_PollEvent(&e);

        if (cur == endIdx) break;
    }

    SDL_Delay(400);

    /* --- Phase 3 : effacement de l'exploration --- */
    for (int i = 0; i < n; i++) overlay[i] = -1;
    drawMaze(ren, g, cs, wt, overlay, startIdx, endIdx);
    SDL_Delay(300);

    /* --- Phase 4 : reconstruction et tracé du chemin (jaune) --- */
    int cur = endIdx;
    while (cur != -1) {
        overlay[cur] = 0xf5c518; /* jaune doré */
        cur = prev[cur];
    }
    /* Forcer la couleur sur le point de départ aussi */
    overlay[startIdx] = 0xf5c518;

    drawMaze(ren, g, cs, wt, overlay, startIdx, endIdx);

    queueFree(&q);
    free(prev);
    free(vis);
    free(overlay);
}

/* ----------------------------------------------------------------
 * main
 * ---------------------------------------------------------------- */

int main(void) {
    srand((unsigned)time(NULL));

    int rows          = 20;
    int cols          = 30;
    int cellSize      = 28;
    int wallThickness = 3;

    int startIdx = 0;                          /* coin haut-gauche  */
    int endIdx   = (rows - 1) * cols + (cols - 1); /* coin bas-droite   */

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow(
        "Labyrinthe — Division récursive + BFS",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        cols * cellSize, rows * cellSize,
        SDL_WINDOW_SHOWN
    );
    SDL_Renderer *ren = SDL_CreateRenderer(
        win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    Grid g = newGrid(cols, rows);

    /* Phase 1 : génération */
    generateMaze(ren, &g, cellSize, wallThickness, startIdx, endIdx);
    SDL_Delay(500);

    /* Phase 2-4 : résolution BFS */
    solveMaze(ren, &g, cellSize, wallThickness, startIdx, endIdx);

    /* Attente fermeture fenêtre */
    SDL_Event e;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT) running = 0;
        SDL_Delay(16);
    }

    freeGrid(&g);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
