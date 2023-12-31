#ifndef GRID_H
#define GRID_H

#define ZWIDTH 16
#define ZHEIGHT 16
#define BLOCK_SIZE (ZWIDTH * ZHEIGHT)

#include <stdint.h>
#include <stddef.h>
#include "raylib.h"

#define align_div(x, div) (((x) / (div)) + (((x) % (div)) ? 1 : 0))

typedef enum {
    NIL = 0,
    LINE = 1,
    LEAD = 2,
    VIA = 3,
} OBSTACLE;

struct point {
    int x;
    int y;
    OBSTACLE obstacle;
};

struct vec2 {
    int x;
    int y;

    implicit operator Vector2(){
        return {
            .x = (float)x,
            .y = (float)y,
        }
    }
};

struct lead {
    struct vec2 orig;
    int width; 
    int height;
};

struct component {
    struct lead leads[2];
};

struct node {
    struct point p;
    float distance;
    int visited;
};

struct zblock {
    struct node nodes[BLOCK_SIZE];
};

struct zgrid {
    size_t nzblocks;
    size_t nwidth;
    size_t nheight;

    size_t width;
    size_t height;

    struct zblock *blocks;
};

#define grid_foreach(node, grid)         \
    struct node *node = grid->blocks[0].nodes;                          \
    for (int i = 0; i < grid->nzblocks * BLOCK_SIZE; i++, node = &grid->blocks[i / BLOCK_SIZE].nodes[i % BLOCK_SIZE])


struct node *get_node(struct zgrid *grid, int x, int y);

void create_zgrid(struct zgrid *grid);

struct node *get_node(struct zgrid *grid, int x, int y);

void delete_zgrid(struct zgrid *grid);

struct point vector_to_point(Vector2 vec);

#endif
