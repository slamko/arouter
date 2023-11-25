#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "raylib.h"

#define GRID_WIDTH 48
#define GRID_HEIGHT 48

const int screen_width = 800;
const int screen_height = 450;

struct point {
    int x;
    int y;
    int obstacle;
};

struct node {
    struct point p;
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
    return (sqrtf(powf(b->x - a->x, 2.) + powf(b->y - a->y, 2.)));
}

int dijkstra(struct circuit *circuit, struct grid *grid) {
    struct node *unvisited = malloc(grid->width * grid->height * sizeof (*unvisited));
    size_t unvisited_num = grid->width * grid->height;

    for (size_t i = 0; i < grid->height * grid->width; i++) {
        unvisited[i] = (struct node) {
            .p = grid->pts[i],
            .visited = false,
            .distance = INFINITY,
        };
    }

    for (struct connection *con = circuit->connects; con - circuit->connects < circuit->num_connects; con++) {
        struct node *first = &unvisited[con->a->y * grid->width + con->a->x];
        struct node *dest = &unvisited[con->b->y * grid->width + con->b->x];

        struct node *current = first;
        first->distance = 0.f;

        while (unvisited_num > 0) {
            struct node *next = NULL;
            float next_dist = INFINITY;

            for (int y = (current->p.y ? -1 : 0); y <= ((current->p.y >= grid->height - 1) ? 0 : 1); y++) {
                for (int x = (current->p.x ? -1 : 0); x <= ((current->p.x >= grid->width - 1) ? 0 : 1); x++) {
                    struct node *node = &unvisited[(current->p.y + y) * grid->width + (current->p.x + x)];

                    if (node->visited || node == current || node->p.obstacle) {
                        continue;
                    }

                    float dist = distance(&node->p, &current->p) + current->distance;

                    if (node->distance > dist) {
                        node->distance = dist;
                    }

                    if (dist < next_dist) {
                        next_dist = dist;
                        next = node;
                    }
                }
            }

            current->visited = true;
            unvisited_num --;
            current = next;

            if (!next) {
                return -1;
            }

            if (current == dest) {
                break;
            }
        }


        while (current != first) {
            struct node *next = NULL;
            float next_dist = INFINITY;

            for (int y = (current->p.y ? -1 : 0); y <= ((current->p.y >= grid->height - 1) ? 0 : 1); y++) {
                for (int x = (current->p.x ? -1 : 0); x <= ((current->p.x >= grid->width - 1) ? 0 : 1); x++) {
                    struct node *node = &unvisited[(current->p.y + y) * grid->width + (current->p.x + x)];

                    if (!node->visited || node == current || node->p.obstacle) {
                        continue;
                    }

                    float dist = node->distance; 

                    if (dist < next_dist) {
                        next_dist = dist;
                        next = node;
                    }
                }
            }

            current = next;

            if (!next) {
                return -1;
            }

            DrawRectangle((next->p.x / (float)GRID_WIDTH) * screen_width, (next->p.y / (float)GRID_HEIGHT) * screen_height, 5, 5, (Color){200, 0, 0, 255});

            if (current == first) {
                break;
            }
 
        }
        
    }

    return 0;
}

void grid_fill(struct grid *grid) {
    for (size_t i = 0; i < grid->height; i++) {
        for (size_t j = 0; j < grid->width; j++) {
            grid->pts[i*grid->width + j] = (struct point) { .x = j, .y = i };
        }
    }
}

int main() {

    struct point pts[GRID_WIDTH * GRID_HEIGHT];
    struct grid grid = {.pts = pts, .width = GRID_WIDTH, .height = GRID_HEIGHT };
    
    grid_fill(&grid);

    struct point a = { .x = 10, .y = 10 };
    struct point b = { .x = 35, .y = 37 };

    struct circuit circ = {0};
    struct connection netlist[1] = {
        {.a = &a, .b = &b },
    };

    circ.connects = netlist;
    circ.num_connects = 1;


    InitWindow(screen_width, screen_height, "Autorouter");

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangle((a.x / (float)GRID_WIDTH) * screen_width, (a.y / (float)GRID_HEIGHT) * screen_height, 5, 5, (Color){200, 0, 0, 255});
        DrawRectangle((b.x / (float)GRID_WIDTH) * screen_width, (b.y / (float)GRID_HEIGHT) * screen_height, 5, 5, (Color){200, 0, 0, 255});

        dijkstra(&circ, &grid);

        EndDrawing();
    }

    CloseWindow();
    
    return 0;
}


