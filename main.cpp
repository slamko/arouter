#include <cmath>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "grid.hpp"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <queue>
#include <vector>

#define GRID_WIDTH 320
#define GRID_HEIGHT 180

const int screen_width = 1280;
const int screen_height = 720;

#define scalex(coord) (int)(((coord) / (float)GRID_WIDTH) * screen_width)
#define scaley(coord) (int)(((coord) / (float)GRID_HEIGHT) * screen_height)

RenderTexture2D target;
Camera2D camera;

typedef struct vec2 vec2;

struct line {
    vec2 start;
    vec2 end;
    std::vector<point> obstacle_points;
};

struct connection;
struct trace;

struct lead {
  struct vec2 orig;
  int width; 
  int height;
  std::vector<trace> traces;
};

struct trace {
  std::vector<line> lines{};
  struct connection *con;
};

struct connection {
  // struct line line;
  struct lead *start;
  struct lead *end;
    // struct trace *trace;
};

struct grid {
  size_t width;
  size_t height;
  struct point *pts;
};

static std::vector<line> lines{};
static std::vector<trace> traces{};
static std::vector<lead> leads{};
static std::vector<connection> connections{};

float euclid_distance(struct point *a, struct point *b) {
    if (a->x == b->x) {
        return abs(b->y - a->y);
    } else if (a->y == b->y) {
        return abs(b->x - a->x); // 
    } else if (abs(b->y - a->y) == 1 && abs(b->x - a->x) == 1) {
        return sqrt(2);
    } else {
        return (sqrt(abs(b->x - a->x) * abs(b->x - a->x) +
                     abs(b->y - a->y) * abs(b->y - a->y)));
    }
}

void draw_path(struct node *first, struct node *dest, struct zgrid *grid) {
    struct node *current = dest;
    int last_x = dest->p.x;
    int last_y = dest->p.y;

    struct vec2 prev_vec {};
    struct vec2 prev_pos = {last_x, last_y};

    struct lead depart = {
        .orig = {scalex(first->p.x) - 5, scaley(first->p.y) - 5},
        .width = 10,
        .height = 10,
    };

    struct lead last = {
        .orig = {scalex(dest->p.x) - 5, scaley(dest->p.y) - 5},
        .width = 10,
        .height = 10,
    };

    // leads.push_back(depart);
    // leads.push_back(last);

    while (current != first) {
        struct node *next = NULL;
        float next_dist = INFINITY;
        std::vector<point> obstacle_points{};

        for (int y = (current->p.y ? -1 : 0);
             y <= ((current->p.y >= grid->height - 1) ? 0 : 1); y++) {
            for (int x = (current->p.x ? -1 : 0);
                 x <= ((current->p.x >= grid->width - 1) ? 0 : 1); x++) {
                struct node *node =
                    get_node(grid, current->p.x + x, current->p.y + y);

                if (node->p.obstacle)
                    continue;

                node->p.obstacle = LINE;
                obstacle_points.push_back(node->p);
                /* DrawRectangle(scalex(node->p.x), scaley(node->p.y), 1, 1,
                 * GREEN); */

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

        if (next->p.x - prev_pos.x != prev_vec.x ||
            next->p.y - prev_pos.y != prev_vec.y || next == first) {
            line new_line = {
                .start = {scalex(current->p.x), scalex(current->p.y)},
                .end = {scalex(last_x), scalex(last_y)},
                .obstacle_points = obstacle_points,
            };

            lines.push_back(new_line);

            prev_vec = {next->p.x - prev_pos.x, next->p.y - prev_pos.y};

            last_x = current->p.x;
            last_y = current->p.y;
        }

        prev_pos = {current->p.x, current->p.y};
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

    return HEURISTIC_D1 * (dx + dy) + (HEURISTIC_D2 - 2 * HEURISTIC_D1) +
           (dx > dy ? dy : dx);
}

int build_work_grid(struct zgrid *grid) { return 0; }

int search(struct node *first, struct node *dest, struct zgrid *grid) {
    std::queue<struct node *> unvisited{};
    unvisited.push(first);

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

                struct node *node =
                    get_node(grid, current->p.x + x, current->p.y + y);

                if (node->p.obstacle) {
                }

                if (node->visited || node == current || node->p.obstacle) {
                    continue;
                }

                float dist =
                    euclid_distance(&node->p, &current->p) + current->distance;

                if (node->distance > dist) {
                    node->distance = dist;
                }

                float f_dist = node->distance + heuristic(node, dest);

                if (next) {
                    unvisited.push(next);
                }

                if (f_dist < next_dist) {
                    next_dist = f_dist;
                    next = node;
                }
            }
        }

        current->visited = true;
        /* DrawRectangle(scalex(current->p.x), scaley(current->p.y), 2, 2, RED);
         */

        unvisited_num--;

        if (!next) {
            fprintf(stderr, "Dijkstra error: unvisited: %zu, point: %d:%d\n",
                    unvisited_num, current->p.x, current->p.y);

            for (; !unvisited.empty(); unvisited.pop()) {
                struct node *visit = unvisited.front();
                if (!visit->visited && !visit->p.obstacle) {
                    next = visit;
                    break;
                }
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

                // fprintf(stderr, "Dijkstra definitif error: unvisited: %zu,
                // point: %d:%d\n", unvisited_num, current->p.x, current->p.y);
                // return -1;
            }
        }

        current = next;

        if (current == dest) {
            break;
        }
    }

    if (unvisited_num == 0) {
        fprintf(stderr, "Dijkstra error: unvisited: %zu, point: %d:%d\n",
                unvisited_num, current->p.x, current->p.y);
        return 1;
    }

    return 0;
}

float total_path_dist(struct node *first, struct node *dest, struct zgrid *grid) {
  struct node *current = dest;
  float dist = 0.0f;

    while (current != first) {
        struct node *next = NULL;
        float next_dist = INFINITY;
        std::vector<point> obstacle_points{};

        for (int y = (current->p.y ? -1 : 0);
             y <= ((current->p.y >= grid->height - 1) ? 0 : 1); y++) {
            for (int x = (current->p.x ? -1 : 0);
                 x <= ((current->p.x >= grid->width - 1) ? 0 : 1); x++) {
                struct node *node =
                    get_node(grid, current->p.x + x, current->p.y + y);

                if (node->p.obstacle)
                    continue;

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

        dist += next_dist;

        if (!next) {
            break;
        }

        current = next;

        if (next == first) {
            break;
        }
    }

    return dist;
}

int dijkstra(std::vector<connection> &connects, struct zgrid *grid) {
    int ret = 0;

    struct zgrid work_grid {};
    if (grid_copy(grid, &work_grid)) {
        return 1;
    }

    for (auto &con : connects) { // 
      struct node *closest_dest {};
      struct node *first = get_node(grid, con.start->orig.x / 4, con.start->orig.y / 4);
      float tot_dist = INFINITY;
      
      for (auto &trace : con.start->traces) {
        for (auto &line : trace.lines) {
          
        struct node *dest = get_node(grid, line.end.x / 4, line.end.y / 4);
        grid_foreach(node, grid) {
            node->visited = false;
            node->distance = INFINITY;
        }

        build_work_grid(&work_grid);

        search(first, dest, &work_grid);

        if (total_path_dist(first, dest, &work_grid) < tot_dist) {
          closest_dest = dest;
        }
        
       /* EndDrawing(); */

        if (ret < 0)
            return ret;
        }
      }

      grid_foreach(node, grid) {
        node->visited = false;
        node->distance = INFINITY;
      }
      
      build_work_grid(&work_grid);

      search(first, closest_dest, &work_grid);
      
      draw_path(first, closest_dest, &work_grid);
      EndMode2D();
      EndTextureMode();
    }

    return ret;
}

int route(std::vector<connection> &circuit, struct zgrid *grid) {
    int ret = 0;
    ret = dijkstra(circuit, grid);
    /* ret |= draw(circuit, grid); */

    return ret;
}

void grid_fill(struct grid *grid) {
    for (size_t i = 0; i < grid->height; i++) {
        for (size_t j = 0; j < grid->width; j++) {
            grid->pts[i * grid->width + j] =
                (struct point){.x = (int)j, .y = (int)i};
        }
    }
}

int add_lead(struct zgrid *circ, struct point pos) {
    for (int y = ((pos.y >= circ->height - 1) ? 0 : 1); y >= (pos.y ? -1 : 0);
         y--) {

        for (int x = (pos.x ? -1 : 0);
             x <= ((pos.x >= circ->width - 1) ? 0 : 1); x++) {

            struct node *node = get_node(circ, pos.x + x, pos.y + y);

            if (node->p.obstacle) {
                return 1;
            }
        }
    }

    for (int y = ((pos.y >= circ->height - 1) ? 0 : 1); y >= (pos.y ? -1 : 0);
         y--) {

        for (int x = (pos.x ? -1 : 0);
             x <= ((pos.x >= circ->width - 1) ? 0 : 1); x++) {

            struct node *node = get_node(circ, pos.x + x, pos.y + y);

            node->p.obstacle = LEAD;
        }
    }

    struct lead new_lead = {
        .orig = {scalex(pos.x) - 5, scaley(pos.y) - 5},
        .width = 10,
        .height = 10,
    };

    leads.push_back(new_lead);

    return 0;
}

int main() {

    struct point pts[GRID_WIDTH * GRID_HEIGHT];
    struct grid grid = {.width = GRID_WIDTH, .height = GRID_HEIGHT, .pts = pts};

    struct zgrid zgrid = {0};
    zgrid.height = GRID_HEIGHT;
    zgrid.width = GRID_WIDTH;

    create_zgrid(&zgrid);

    grid_fill(&grid);

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

    printf("Elapsed: %fus\n",
           (float)((end - start) * 1000 * 1000) / CLOCKS_PER_SEC);

    bool add_connection_mode = false;
    bool add_lien_mode = false;
    int first_point = false;

    vec2 last_point{};
    vec2 last_lead{};
    bool x_coord = false, y_coord = false;
    int x_input{}, y_input{};

    char text[64] = {0};
    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        if (IsKeyPressed(KEY_A)) {
            add_connection_mode = true;
        }

        if (IsKeyPressed(KEY_L)) {
            add_lien_mode = true;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            add_connection_mode = false;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && add_connection_mode) {
            if (first_point) {
                vec2 dest = GetScreenToWorld2D(GetMousePosition(), camera);
                printf("Hello %d, %d\n", dest.x, dest.y);
                printf("Last %d, %d\n", last_point.x, last_point.y);

                std::vector<line> connects = {{last_point, dest}};
                int ret = route(connects, &zgrid);

                first_point = false;
                add_connection_mode = false;
                last_point = {};

            } else {
                last_point = GetScreenToWorld2D(GetMousePosition(), camera); //
                first_point = true;
            }
            printf("World %d, %d\n",
                   (int)GetScreenToWorld2D(GetMousePosition(), camera).x,
                   (int)GetScreenToWorld2D(GetMousePosition(), camera).y);

        } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && add_lien_mode) {
            Vector2 pos = GetScreenToWorld2D(GetMousePosition(), camera);
            for (auto &lead : leads) {
                if (CheckCollisionPointRec(pos,
                                           (Rectangle){
                                               .x = (float)lead.orig.x - 5,
                                               .y = (float)lead.orig.y - 5,
                                               .width = (float)lead.width,
                                               .height = (float)lead.height,
                                           })) {
                    if (first_point) {
                        struct vec2 dest_lead = lead.orig;
                        connections.push_back(
                            (line){.start = last_lead, .end = dest_lead});
                        last_lead = {};
                        first_point = false;
                    } else {
                        first_point = true;
                        last_lead = lead.orig;
                    }
                }
            }
        } else if (IsKeyPressed(KEY_C)) {
            x_coord = true;
        } else if (IsKeyPressed(KEY_R)) {
            route(connections, &zgrid);
            connections.clear();
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / camera.zoom);

            camera.target = Vector2Add(camera.target, delta);
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            Vector2 mouseWorldPos =
                GetScreenToWorld2D(GetMousePosition(), camera);

            camera.offset = GetMousePosition();

            camera.target = mouseWorldPos;

            const float zoomIncrement = 0.2f;

            camera.zoom += (wheel * zoomIncrement);
            if (camera.zoom < zoomIncrement)
                camera.zoom = zoomIncrement;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);
        rlPushMatrix();
        rlTranslatef(0, 25 * 50, 0);
        rlRotatef(90, 1, 0, 0);
        DrawGrid(100, 50);
        rlPopMatrix();

        DrawTextureRec(target.texture,
                       (Rectangle){0, 0, (float)target.texture.width,
                                   (float)-target.texture.height},
                       (Vector2){0, 0}, WHITE);

        for (auto &line : lines) {
            DrawLineEx({(float)line.start.x, (float)line.start.y},
                       {(float)line.end.x, (float)line.end.y}, 3.0, BLUE); //
        }

        for (auto &line : connections) {
            DrawLineEx({(float)line.start.x, (float)line.start.y},
                       {(float)line.end.x, (float)line.end.y}, 1.2, GREEN); //
        }

        for (auto &lead : leads) {
            DrawRectangleV(
                (Vector2){(float)lead.orig.x - 5, (float)lead.orig.y - 5},
                (Vector2){(float)lead.width, (float)lead.height},
                (Color){200, 0, 0, 255});
        }

        EndMode2D();

        if (y_coord) {

            int ret = GuiTextInputBox(
                {.x = 550, .y = 340, .width = 150, .height = 150}, "New lead",
                "Y coordinate", "Apply", text, 18, NULL);
            y_input = atoi(text);

            if (ret >= 0) {
                y_coord = false;

                if (ret == 1) {
                    if (add_lead(&zgrid,
                                 vector_to_point((vec2){x_input, y_input}))) {
                        printf("Add lead failed\n");
                    }
                }
            }
        }

        if (x_coord) {

            int ret = GuiTextInputBox(
                {.x = 550, .y = 340, .width = 150, .height = 150}, "New lead",
                "X coordinate", "Apply", text, 18, NULL);
            x_input = atoi(text);

            if (ret >= 0) {
                x_coord = false;

                if (ret == 1) {
                    y_coord = true;
                }
            }
        }

        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
    delete_zgrid(&zgrid);

    return 0;
}
