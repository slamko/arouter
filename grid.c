#include "grid.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

void delete_zgrid(struct zgrid *grid) {
    free(grid->blocks);
}

void create_zgrid(struct zgrid *grid) {
    size_t nzblocks = align_div(grid->width, ZWIDTH) * align_div(grid->height, ZHEIGHT);

    grid->blocks = malloc(sizeof (*grid->blocks) * nzblocks);

    grid->nwidth = align_div(grid->width, ZWIDTH);
    grid->nheight = align_div(grid->height, ZHEIGHT);
    grid->nzblocks = grid->nwidth * grid->nheight;

    for (int z = 0; z < nzblocks; z++) {
        for (int i = 0; i < ZWIDTH * ZHEIGHT; i++) {
            struct point p = (struct point) {
                .x = (i % ZWIDTH) + ((z % grid->nwidth) * ZWIDTH),
                .y = (i / ZWIDTH) + ((z / grid->nwidth) * ZHEIGHT),
                .obstacle = false,
            };

            grid->blocks[z].nodes[i] = (struct node) {
                .p = p,
                .visited = false,
                .distance = INFINITY,
            };
                
        }
    }
}

struct node *get_node(struct zgrid *grid, int x, int y) {
    struct zblock *block = &grid->blocks[(x/ZWIDTH) + (y/ZHEIGHT) * (align_div(grid->width, ZWIDTH))];
    return &block->nodes[(x % ZWIDTH) + (y % ZHEIGHT) * ZWIDTH];
}

