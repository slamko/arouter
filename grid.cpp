#include "grid.hpp"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "raylib.h"

void delete_zgrid(struct zgrid *grid) {
    delete[] grid->blocks;
}

int grid_copy(struct zgrid *grid, struct zgrid *new_grid) {
  *new_grid = *grid;
  new_grid->blocks = new zblock[new_grid->nzblocks];

  if (!new_grid->blocks) {
    return 1;
  }
  
  for (size_t i = 0; i < grid->nzblocks; i++) {
    std::copy(std::begin(grid->blocks[i].nodes), std::end(grid->blocks[i].nodes), std::begin(new_grid->blocks[i].nodes));
  }

  return 0;
}

void create_zgrid(struct zgrid *grid) {
    size_t nzblocks = align_div(grid->width, ZWIDTH) * align_div(grid->height, ZHEIGHT);

    grid->blocks = new zblock[nzblocks];

    grid->nwidth = align_div(grid->width, ZWIDTH);
    grid->nheight = align_div(grid->height, ZHEIGHT);
    grid->nzblocks = grid->nwidth * grid->nheight;

    for (int z = 0; z < nzblocks; z++) {
        for (int i = 0; i < ZWIDTH * ZHEIGHT; i++) {
            struct point p = (struct point) {
              .x = (int)((i % ZWIDTH) + ((z % grid->nwidth) * ZWIDTH)),
              .y = (int)((i / ZWIDTH) + ((z / grid->nwidth) * ZHEIGHT)),
                .obstacle = NIL,
            };

            grid->blocks[z].nodes[i] = (struct node) {
                .p = p,
                .distance = INFINITY,
                .visited = true,
            };
                
        }
    }
}

struct node *get_node(struct zgrid *grid, int x, int y) {
    struct zblock *block = &grid->blocks[(x/ZWIDTH) + (y/ZHEIGHT) * (align_div(grid->width, ZWIDTH))];
    return &block->nodes[(x % ZWIDTH) + (y % ZHEIGHT) * ZWIDTH];
}

vec2 scale_vec(vec2 vec) {
  return {
    vec.x / 4,
    vec.y / 4,
  };
}


struct point vector_to_point(Vector2 vec) {
    return (struct point) {
      .x = (int)vec.x / 4,
      .y = (int)vec.y / 4,
      .obstacle = NIL,
    };
}
