#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "raylib.h"

#define GRID_WIDTH 320
#define GRID_HEIGHT 180

const int screen_width = 1280;
const int screen_height = 720;

#define scalex(coord) ((coord / (float)GRID_WIDTH) * screen_width)
#define scaley(coord) ((coord / (float)GRID_HEIGHT) * screen_height)

struct point {
    int x;
    int y;
    int obstacle;
};

struct node {
    struct point *p;
    float distance;
    int visited;
};

struct grid {
    size_t width;
    size_t height;
    struct point *pts;
};

struct connection {
    struct point *a, *b;
};

struct circuit {
    struct connection *connects;
    size_t num_connects;
};

float distance(struct point *a, struct point *b) {
    if (a->x == b->x) {
        return abs(b->y - a->y);
    } else if (a->y == b->y) {
        return abs(b->x - a->x);
    } else if (abs(b->y - a->y) == 1 && abs(b->x - a->x) == 1) {
        return sqrt(2);
    } else{
        return (sqrt(pow(b->x - a->x, 2) + pow(b->y - a->y, 2)));
    }
}

void draw_path(struct node *nodes, struct node *first, struct node *dest,
               struct grid *grid) {
    struct node *current = dest;
    int last_x = dest->p->x;
    int last_y = dest->p->y;

    while (current != first) {
        struct node *next = NULL;
        float next_dist = INFINITY;

        for (int y = (current->p->y ? -1 : 0);
             y <= ((current->p->y >= grid->height - 1) ? 0 : 1); y++) {
            for (int x = (current->p->x ? -1 : 0);
                 x <= ((current->p->x >= grid->width - 1) ? 0 : 1); x++) {
                struct node *node = &nodes[(current->p->y + y) * grid->width +
                                           (current->p->x + x)];

                if (node->p->obstacle) continue;

                node->p->obstacle = true;

                if (!node->visited || node == current) {
                    continue;
                }


                float dist = node->distance;

                if (dist < next_dist) {
                    next_dist = dist;
                    next = node;
                }
            }
        }

        if (next == first) {
            break;
        }

        if (!next) {
            break;
        }

        if (next->p->x == last_x || next->p->y == last_y) {
            current = next;
            continue;
        }

        DrawLineEx((Vector2) {scalex(last_x), scaley(last_y)}, (Vector2) {scalex(current->p->x), scaley(current->p->y) }, 3., BLUE);
        last_x = current->p->x;
        last_y = current->p->y;

        current = next;

    }
}

#define HEURISTIC_D1 0.95
#define HEURISTIC_D2 0.707

float heuristic(struct node *node, struct node *dest) {
    float dx = abs(node->p->x - dest->p->x);
    float dy = abs(node->p->y - dest->p->y);

    return HEURISTIC_D1 * (dx + dy) + (HEURISTIC_D2 - 2 * HEURISTIC_D1) + (dx > dy ? dy : dx);
}

int dijkstra(struct circuit *circuit, struct grid *grid) {
    int ret = 0;
    struct node *unvisited =
        malloc(grid->width * grid->height * sizeof(*unvisited));

    for (size_t i = 0; i < grid->height * grid->width; i++) {
        unvisited[i] = (struct node) {
            .p = &grid->pts[i],
            .visited = false,
            .distance = INFINITY,
        };
    }

    for (struct connection *con = circuit->connects;
         con - circuit->connects < circuit->num_connects; con++) {
        struct node *first = &unvisited[con->a->y * grid->width + con->a->x];
        struct node *dest = &unvisited[con->b->y * grid->width + con->b->x];
        size_t unvisited_num = grid->width * grid->height;

        struct node *current = first;
        first->distance = 0.f;

        while (unvisited_num > 0) {
            struct node *next = NULL;
            float next_dist = INFINITY;

            for (int y = (current->p->y ? -1 : 0);
                 y <= ((current->p->y >= grid->height - 1) ? 0 : 1); y++) {
                for (int x = (current->p->x ? -1 : 0);
                     x <= ((current->p->x >= grid->width - 1) ? 0 : 1); x++) {
                    struct node *node =
                        &unvisited[(current->p->y + y) * grid->width +
                                   (current->p->x + x)];

                    if (node->visited || node == current || node->p->obstacle) {
                        continue;
                    }

                    float dist = distance(node->p, current->p) + current->distance;

                    if (node->distance > dist) {
                        node->distance = dist;
                    }

                    float f_dist = dist + heuristic(node, dest);
                    if (f_dist < next_dist) {
                        next_dist = f_dist;
                        next = node;
                    }
                }
            }

            current->visited = true;
            unvisited_num--;

            if (!next) {
                fprintf(stderr, "Dijkstra error");
                ret = -1;
            }

            current = next;

            if (current == dest) {
                break;
            }
        }

        draw_path(unvisited, first, dest, grid);

        for (size_t i = 0; i < grid->height * grid->width; i++) {
            unvisited[i].visited = false;
            unvisited[i].distance = INFINITY;
        }
    }

  cleanup:
    free(unvisited);

    return ret;
}

void grid_fill(struct grid *grid) {
    for (size_t i = 0; i < grid->height; i++) {
        for (size_t j = 0; j < grid->width; j++) {
            grid->pts[i * grid->width + j] = (struct point){.x = j, .y = i};
        }
    }
}

int main() {

    struct point pts[GRID_WIDTH * GRID_HEIGHT];
    struct grid grid = {.pts = pts, .width = GRID_WIDTH, .height = GRID_HEIGHT};

    grid_fill(&grid);

    struct point a = {.x = 50, .y = 80};
    struct point b = {.x = 270, .y = 170};

    struct point c = {.x = 100, .y = 130};
    struct point d = {.x = 300, .y = 10};

    struct circuit circ = {0};
    struct connection netlist[] = {
        {.a = &b, .b = &a},
        {.a = &c, .b = &d},
    };

    circ.connects = netlist;
    circ.num_connects = sizeof(netlist) / sizeof(*netlist);

    InitWindow(screen_width, screen_height, "Autorouter");

    RenderTexture2D target = LoadRenderTexture(screen_width, screen_height);

    BeginTextureMode(target);

    for (size_t i = 0; i < sizeof netlist / sizeof *netlist; i++) {
        DrawRectangle((netlist[i].a->x / (float)GRID_WIDTH) * screen_width - 5,
                      (netlist[i].a->y / (float)GRID_HEIGHT) * screen_height - 5, 10,
                      10, (Color){200, 0, 0, 255});
        DrawRectangle((netlist[i].b->x / (float)GRID_WIDTH) * screen_width - 5,
                      (netlist[i].b->y / (float)GRID_HEIGHT) * screen_height - 5, 10,
                      10, (Color){200, 0, 0, 255});
    }

    ClearBackground(RAYWHITE);

    int ret = dijkstra(&circ, &grid);

    EndTextureMode();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawTextureRec(target.texture,
                       (Rectangle){0, 0, (float)target.texture.width,
                                   (float)-target.texture.height},
                       (Vector2){0, 0}, WHITE);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
