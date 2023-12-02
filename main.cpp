#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <vector>
#include "grid.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#define GRID_WIDTH 320
#define GRID_HEIGHT 180

const int screen_width = 1280;
const int screen_height = 720;

#define scalex(coord) ((coord / (float)GRID_WIDTH) * screen_width)
#define scaley(coord) ((coord / (float)GRID_HEIGHT) * screen_height)

RenderTexture2D target;
Camera2D camera;

typedef struct vec2 vec2;

struct line {
    vec2 start;
    vec2 end;
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
        return (sqrt(abs(b->x - a->x) * abs(b->x - a->x) + abs(b->y - a->y) * abs(b->y - a->y)));
    }
}

void draw_path(struct node *first, struct node *dest, struct zgrid *grid) {
    struct node *current = dest;
    int last_x = dest->p.x;
    int last_y = dest->p.y;

    struct vec2 prev_vec = {0};
    struct vec2 prev_pos = {last_x, last_y};

    DrawRectangle((dest->p.x / (float)GRID_WIDTH) * screen_width - 5,
                      (dest->p.y / (float)GRID_HEIGHT) * screen_height - 5, 10,
                      10, (Color){200, 0, 0, 255});

    DrawRectangle((first->p.x / (float)GRID_WIDTH) * screen_width - 5,
                      (first->p.y / (float)GRID_HEIGHT) * screen_height - 5, 10,
                      10, (Color){200, 0, 0, 255});


    while (current != first) {
        struct node *next = NULL;
        float next_dist = INFINITY;

        for (int y = (current->p.y ? -1 : 0);
             y <= ((current->p.y >= grid->height - 1) ? 0 : 1); y++) {
            for (int x = (current->p.x ? -1 : 0);
                 x <= ((current->p.x >= grid->width - 1) ? 0 : 1); x++) {
                struct node *node = get_node(grid, current->p.x + x, current->p.y + y);

                if (node->p.obstacle) continue;

                node->p.obstacle = true;
                /* DrawRectangle(scalex(node->p.x), scaley(node->p.y), 1, 1, GREEN); */

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

        if (!next) {
            break;
        }

        if (next->p.x - prev_pos.x != prev_vec.x || next->p.y - prev_pos.y != prev_vec.y || next == first) {

            DrawLineEx(GetScreenToWorld2D((Vector2) {scalex(last_x), scaley(last_y)}, camera),
                       GetScreenToWorld2D((Vector2) {scalex(current->p.x), scaley(current->p.y) }, camera), 3. / camera.zoom, BLUE);

            prev_vec = (struct vec2) { next->p.x - prev_pos.x, next->p.y - prev_pos.y};

            last_x = current->p.x;
            last_y = current->p.y;
        }

        prev_pos = (struct vec2) { current->p.x, current->p.y };
        current = next;

        if (next == first) {
            break;
        }
    }
}

#define HEURISTIC_D1 0.15
#define HEURISTIC_D2 0.106

float heuristic(struct node *node, struct node *dest) {
    float dx = abs(node->p.x - dest->p.x);
    float dy = abs(node->p.y - dest->p.y);

    return HEURISTIC_D1 * (dx + dy) + (HEURISTIC_D2 - 2 * HEURISTIC_D1) + (dx > dy ? dy : dx);
    /* return 0.f; */
}

int dijkstra(struct circuit *circuit, struct zgrid *grid) {
    int ret = 0;

    for (struct connection *con = circuit->connects;
         con - circuit->connects < circuit->num_connects; con++) {

        grid_foreach(node, grid) {
            node->visited = false;
            node->distance = INFINITY;
        }

        struct node *first = get_node(grid, con->a->x, con->a->y);
        struct node *dest = get_node(grid, con->b->x, con->b->y);

        struct llist *unvisited = calloc(1, sizeof *unvisited);
        // std::ve
        struct llist *cur_unvisited = unvisited;

        if (first == dest) {
            return 0;
        }

        size_t unvisited_num = grid->width * grid->height;

        struct node *current = first;
        first->distance = 0.f;

        BeginTextureMode(target);
        BeginMode2D(camera);
        while (unvisited_num > 0) {
            struct node *next = NULL;
            float next_dist = INFINITY;

            for (int y = ((current->p.y >= grid->height - 1) ? 0 : 1);
                 y >= (current->p.y ? -1 : 0); y--) {

                for (int x = (current->p.x ? -1 : 0);
                     x <= ((current->p.x >= grid->width - 1) ? 0 : 1); x++) {

                    struct node *node = get_node(grid, current->p.x + x, current->p.y + y);
                    
                    if (node->p.obstacle) {
                    }

                    if (node->visited || node == current || node->p.obstacle) {
                        continue;
                    }

                    float dist = distance(&node->p, &current->p) + current->distance;

                    if (node->distance > dist) {
                        node->distance = dist;
                    }

                    float f_dist = node->distance + heuristic(node, dest);

                    if (next) {
                        unvisited->node = next;
                        unvisited->next = calloc(1, sizeof *unvisited);
                        unvisited = unvisited->next;
                    }

                    if (f_dist < next_dist) {
                        next_dist = f_dist;
                        next = node;
                    }


                }
            }

            current->visited = true;
            /* DrawRectangle(scalex(current->p.x), scaley(current->p.y), 2, 2, RED); */

            unvisited_num--;

            if (!next) {
                fprintf(stderr, "Dijkstra error: unvisited: %zu, point: %d:%d\n", unvisited_num, current->p.x, current->p.y);

                struct llist *prev = NULL;
                for (struct llist *visit = cur_unvisited; visit && visit->node; visit = visit->next) {
                    if (!visit->node->visited && !visit->node->p.obstacle) {
                        next = visit->node;
                        break;
                    } else {
                        free(prev);
                        prev = visit;
                    }
                    cur_unvisited = visit;
                }

                if (!next) {

                    grid_foreach(node, grid) {
                        node->visited = false;
                        node->distance = INFINITY;
                    }

                    unvisited_num = grid->width * grid->height;
                    current->distance = 0.;
                    current->visited = true;
                    continue;

                    /* fprintf(stderr, "Dijkstra definitif error: unvisited: %zu, point: %d:%d\n", unvisited_num, current->p.x, current->p.y); */
                    /* ret = -1; */
                    /* goto cleanup; */
                }
            }

            current = next;

            if (current == dest) {
                break;
            }

        }

        if (unvisited_num == 0) {
           fprintf(stderr, "Dijkstra error: unvisited: %zu, point: %d:%d\n", unvisited_num, current->p.x, current->p.y);
           return 1;
        }

        draw_path(first, dest, grid);
        EndMode2D();
        EndTextureMode();
        /* EndDrawing(); */

      cleanup:;
        struct llist *prev = NULL;
        for (struct llist *visit = cur_unvisited; visit; visit = visit->next) {
            free(prev);
            prev = visit;
        }

        if (ret < 0) return ret;
    }

    return ret;
}

int route(struct circuit *circuit, struct zgrid *grid) {
    int ret = 0;
    ret = dijkstra(circuit, grid);
    /* ret |= draw(circuit, grid); */

    return ret;
}

void grid_fill(struct grid *grid) {
    for (size_t i = 0; i < grid->height; i++) {
        for (size_t j = 0; j < grid->width; j++) {
            grid->pts[i * grid->width + j] = (struct point){.x = j, .y = i};
        }
    }
}

int add_lead(struct zgrid *circ, struct point pos) {
    for (int y = ((pos.y >= circ->height - 1) ? 0 : 1);
         y >= (pos.y ? -1 : 0); y--) {
        
        for (int x = (pos.x ? -1 : 0);
             x <= ((pos.x >= circ->width - 1) ? 0 : 1); x++) {

            struct node *node = get_node(circ, pos.x + x, pos.y + y);

            if (node->p.obstacle) {
                return 1;
            }
        }
    }
            
    for (int y = ((pos.y >= circ->height - 1) ? 0 : 1);
         y >= (pos.y ? -1 : 0); y--) {
        
        for (int x = (pos.x ? -1 : 0);
             x <= ((pos.x >= circ->width - 1) ? 0 : 1); x++) {

            struct node *node = get_node(circ, pos.x + x, pos.y + y);

            node->p.obstacle = LEAD;
        }
    }
    
    
    BeginTextureMode(target);
    DrawRectangleV(GetScreenToWorld2D((Vector2) {scalex(pos.x) - 5, scaley(pos.y) - 5}, camera),
                   (Vector2) {10, 10}, (Color){200, 0, 0, 255});
    EndTextureMode();


    return 0;
}

int main() {

    struct point pts[GRID_WIDTH * GRID_HEIGHT];
    struct grid grid = {.pts = pts, .width = GRID_WIDTH, .height = GRID_HEIGHT};

    struct zgrid zgrid = {0};
    zgrid.height = GRID_HEIGHT;
    zgrid.width = GRID_WIDTH;

    create_zgrid(&zgrid);

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
    camera = (Camera2D){0};
    camera.zoom = 1.0f;

    BeginTextureMode(target);
    ClearBackground(RAYWHITE);
    EndTextureMode();
    
    target = LoadRenderTexture(screen_width, screen_height);
    clock_t start = clock();
    /* int ret = route(&circ, &zgrid); */
    clock_t end = clock();

    printf("Elapsed: %fus\n", (float)((end - start) * 1000 * 1000) / CLOCKS_PER_SEC);

    int add_connection_mode = false;
    int first_point = false;
    struct point last_point = {0};
    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        if (IsKeyPressed(KEY_A)) {
            add_connection_mode = true;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            add_connection_mode = false;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && add_connection_mode) {
            if (first_point) {
                struct point dest = vector_to_point(GetScreenToWorld2D(GetMousePosition(), camera));
                printf("Hello %d, %d\n", dest.x, dest.y);
                printf("Last %d, %d\n", last_point.x, last_point.y);

                struct circuit circ = {0};
                struct connection netlist[] = {
                    {.a = &dest, .b = &last_point},
                };
                
                circ.connects = netlist; /*  */
                circ.num_connects = sizeof(netlist) / sizeof(*netlist);
                int ret = route(&circ, &zgrid);

                first_point = false;
                add_connection_mode = false;
                last_point = (struct point){0};


            } else {
                last_point = vector_to_point(GetScreenToWorld2D(GetMousePosition(), camera));
                first_point = true;
            }
            printf("World %d, %d\n", (int)GetScreenToWorld2D(GetMousePosition(), camera).x, (int)GetScreenToWorld2D(GetMousePosition(), camera).y);


        } else if (IsKeyPressed(KEY_C)) {
            Vector2 mouse = GetMousePosition();
            if (add_lead(&zgrid, vector_to_point(GetScreenToWorld2D(GetMousePosition(), camera)))) {
                printf("Add lead failed\n");
            }
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / camera.zoom);

            camera.target = Vector2Add(camera.target, delta);
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0)
        {
            Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
            
            camera.offset = GetMousePosition();

            camera.target = mouseWorldPos;

            const float zoomIncrement = 0.2f;

            camera.zoom += (wheel * zoomIncrement);
            if (camera.zoom < zoomIncrement) camera.zoom = zoomIncrement;
        }

        BeginTextureMode(target);
        ClearBackground(RAYWHITE);
        BeginMode2D(camera);

        DrawLineEx((Vector2) {6, 6} , (Vector2) { 180, 180}, 4.0, RED);

        EndMode2D();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);
        rlPushMatrix();
        rlTranslatef(0, 25*50, 0);
        rlRotatef(90, 1, 0, 0);
        DrawGrid(100, 50);
        rlPopMatrix();
        
        DrawTextureRec(target.texture,
                       (Rectangle){0, 0, (float)target.texture.width,
                                   (float)-target.texture.height},
                       (Vector2){0, 0}, WHITE);
        EndMode2D();

        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
    delete_zgrid(&zgrid);

    return 0;
}
