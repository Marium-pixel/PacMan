#include "raylib.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <array>
#include <string>
#include <cmath>
using namespace std;

constexpr unsigned char CELL_SIZE = 16;
constexpr unsigned char MAP_HEIGHT = 21;
constexpr unsigned char MAP_WIDTH = 21;

// -------------------- Linked List for Pellets --------------------
struct CoinNode {
    int x, y;
    CoinNode* next;
};

class CoinList {
public:
    CoinNode* head = nullptr;

    void addCoin(int x, int y) {
        CoinNode* newCoin = new CoinNode{ x, y, head };
        head = newCoin;
    }

    void drawCoins(int tileSize) {
        CoinNode* curr = head;
        while (curr) {
            int cx = curr->x * tileSize + tileSize / 2;
            int cy = curr->y * tileSize + tileSize / 2;
            DrawCircle(cx, cy, tileSize * 0.15f, ORANGE);
            curr = curr->next;
        }
    }

    void eatCoinAt(int gridX, int gridY) {
        CoinNode* curr = head;
        CoinNode* prev = nullptr;
        while (curr) {
            if (curr->x == gridX && curr->y == gridY) {
                if (prev) prev->next = curr->next;
                else head = curr->next;
                delete curr;
                return;
            }
            prev = curr;
            curr = curr->next;
        }
    }
};

// -------------------- Map Class --------------------
class Map {
public:
    vector<string> layout;
    int rows, cols, tileSize;
    CoinList coins;

    Map(vector<string> mapLayout, int tSize)
        : layout(mapLayout), tileSize(tSize)
    {
        rows = layout.size();
        cols = layout[0].size();

        // Store pellet positions
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (layout[y][x] == '.') coins.addCoin(x, y);
            }
        }
    }

    void Draw() {
        int pad = 0;

        // Draw background (dark corridors)
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                char c = layout[y][x];
                int px = x * tileSize;
                int py = y * tileSize;

                if (c != '#')
                    DrawRectangle(px, py, tileSize, tileSize, BLACK);
            }
        }

        // Draw solid connected walls (closed outlines)
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (layout[y][x] == '#') {
                    int px = x * tileSize;
                    int py = y * tileSize;
                    int w = tileSize;
                    int h = tileSize;

                    if (y == 0 || layout[y - 1][x] != '#')
                        DrawLineEx({ (float)px, (float)py },
                            { (float)(px + w), (float)py }, 3, DARKBLUE);
                    if (y == rows - 1 || layout[y + 1][x] != '#')
                        DrawLineEx({ (float)px, (float)(py + h) },
                            { (float)(px + w), (float)(py + h) }, 3, DARKBLUE);
                    if (x == 0 || layout[y][x - 1] != '#')
                        DrawLineEx({ (float)px, (float)py },
                            { (float)px, (float)(py + h) }, 3, DARKBLUE);
                    if (x == cols - 1 || layout[y][x + 1] != '#')
                        DrawLineEx({ (float)(px + w), (float)py },
                            { (float)(px + w), (float)(py + h) }, 3, DARKBLUE);
                }
            }
        }

        // Draw Ghost Home box
        int gxmin = cols, gxmax = -1, gymin = rows, gymax = -1;
        for (int y = 0; y < rows; y++)
            for (int x = 0; x < cols; x++)
                if (layout[y][x] == 'G') {
                    gxmin = min(gxmin, x);
                    gxmax = max(gxmax, x);
                    gymin = min(gymin, y);
                    gymax = max(gymax, y);
                }

        if (gxmax >= gxmin && gymax >= gymin) {
            int left = gxmin * tileSize + pad;
            int top = gymin * tileSize + pad;
            int w = (gxmax - gxmin + 1) * tileSize - pad;
            int h = (gymax - gymin + 1) * tileSize - pad;
            DrawRectangle(left, top, w, h, Color{ 15,15,15,255 });
            DrawRectangleLinesEx({ (float)left,(float)top,(float)w,(float)h }, 2, DARKBLUE);
        }

        // Draw small pellets
        coins.drawCoins(tileSize);

        // Draw large pellets
        for (int y = 0; y < rows; y++)
            for (int x = 0; x < cols; x++)
                if (layout[y][x] == 'O') {
                    int cx = x * tileSize + tileSize / 2;
                    int cy = y * tileSize + tileSize / 2;
                    DrawCircle(cx, cy, tileSize * 0.25f, ORANGE);
                }

        // Ghost gate
        int gateX = 10 * tileSize;
        int gateY = 9 * tileSize;
        int gateWidth = tileSize * 1;
        DrawLineEx({ (float)gateX,(float)gateY },
            { (float)(gateX + gateWidth),(float)gateY }, 3.5f, SKYBLUE);
    }

    bool isWall(int gx, int gy) {
        if (gx < 0 || gx >= cols || gy < 0 || gy >= rows) return true;
        char c = layout[gy][gx];
        // Treat ghost gate line (y=9, x=10 in your map) as wall
        if (c == '#' || (gy == 9 && gx == 10)) return true;
        return false;
    }

    void eatLargePelletAt(int gx, int gy) {
        if (gx >= 0 && gx < cols && gy >= 0 && gy < rows &&
            layout[gy][gx] == 'O')
            layout[gy][gx] = ' ';
    }
};

enum Cell
{
    Door,
    Empty,
    Energizer,
    Pellet,
    Wall
};



// -------------------- Pacman Class --------------------
class Pacman {
    bool animation_over;
    bool dead;

    unsigned char direction;  // 0=left, 1=right, 2=up, 3=down

    unsigned short animation_timer;
    unsigned short energizer_timer;

public:
    float x, y;
    int tileSize;
    float speed;
    float radius;

    Pacman(int startX, int startY, int tSize)
        : x((float)startX), y((float)startY), tileSize(tSize),
        speed(2.5f), animation_over(false), dead(false),
        direction(1), animation_timer(0), energizer_timer(0)
    {
        radius = tileSize * 0.4f;
    }

    static bool circleIntersectsRect(float cx, float cy, float r,
        float rx, float ry, float rw, float rh) {
        float nearestX = max(rx, min(cx, rx + rw));
        float nearestY = max(ry, min(cy, ry + rh));
        float dx = cx - nearestX;
        float dy = cy - nearestY;
        return (dx * dx + dy * dy) < (r * r);
    }

    bool collidesAtCenter(Map& map, float centerX, float centerY) {
        int gx = (int)(centerX / tileSize);
        int gy = (int)(centerY / tileSize);

        for (int oy = -1; oy <= 1; oy++)
            for (int ox = -1; ox <= 1; ox++) {
                int nx = gx + ox;
                int ny = gy + oy;
                if (map.isWall(nx, ny)) {
                    float rx = nx * tileSize;
                    float ry = ny * tileSize;
                    if (circleIntersectsRect(centerX, centerY, radius,
                        rx, ry, (float)tileSize, (float)tileSize))
                        return true;
                }
            }
        return false;
    }

    // ---------------- Update ----------------
    void update(Map& map) {
        // Snap to center of current grid cell for smooth alignment
        int gridX = (int)((x + tileSize / 2) / tileSize);
        int gridY = (int)((y + tileSize / 2) / tileSize);
        float centerX = gridX * tileSize + tileSize / 2.0f;
        float centerY = gridY * tileSize + tileSize / 2.0f;

        if (fabs(x - centerX) < 1.5f) x = centerX;
        if (fabs(y - centerY) < 1.5f) y = centerY;

        // Handle input
        float dx = 0, dy = 0;
        if (IsKeyDown(KEY_RIGHT)) { dx += speed; direction = 1; }
        if (IsKeyDown(KEY_LEFT)) { dx -= speed; direction = 0; }
        if (IsKeyDown(KEY_UP)) { dy -= speed; direction = 3; }
        if (IsKeyDown(KEY_DOWN)) { dy += speed; direction = 2; }

        // Normalize diagonal movement
        if (dx != 0 && dy != 0) {
            float inv = 1.0f / sqrtf(dx * dx + dy * dy);
            dx *= inv * speed;
            dy *= inv * speed;
        }

        // Check collisions before moving
        float tryX = x + dx;
        float tryY = y + dy;
        if (!collidesAtCenter(map, tryX + tileSize * 0.5f, y + tileSize * 0.5f)) x = tryX;
        if (!collidesAtCenter(map, x + tileSize * 0.5f, tryY + tileSize * 0.5f)) y = tryY;

        // Eat pellets
        int eatGX = (int)((x + tileSize * 0.5f) / tileSize);
        int eatGY = (int)((y + tileSize * 0.5f) / tileSize);
        map.coins.eatCoinAt(eatGX, eatGY); // small pellet
        map.eatLargePelletAt(eatGX, eatGY); // big pellet

        // Update energizer timer
        if (energizer_timer > 0) energizer_timer--;

        animation_timer++;
    }

    // Accessors
    bool get_animation_over() { return animation_over; }
    bool get_dead() { return dead; }
    unsigned char get_direction() { return direction; }
    unsigned short get_energizer_timer() { return energizer_timer; }

    // Mutators
    void set_dead(bool i_dead) { dead = i_dead; }
    void set_animation_timer(unsigned short i_animation_timer) { animation_timer = i_animation_timer; }
    void set_position(short i_x, short i_y) { x = i_x; y = i_y; }

    // Reset Pacman state
    void reset() {
        animation_over = false;
        dead = false;
        animation_timer = 0;
        energizer_timer = 0;
        direction = 1;
    }

    
    // Draw Pacman
    void draw(bool victory = false) {
        float cx = x + tileSize / 2;
        float cy = y + tileSize / 2;

        Color pacColor = victory ? GOLD : YELLOW;
        DrawCircle((int)cx, (int)cy, radius, pacColor);

        // Simple mouth animation
        float mouthAngle = 40 * sin(animation_timer * 0.15f);
        switch (direction) {
        case 0: DrawCircleSector({ cx, cy }, radius, 180 + mouthAngle, 180 - mouthAngle, 0, BLACK); break;
        case 1: DrawCircleSector({ cx, cy }, radius, mouthAngle, -mouthAngle, 0, BLACK); break;
        case 2: DrawCircleSector({ cx, cy }, radius, 90 + mouthAngle, 90 - mouthAngle, 0, BLACK); break;
        case 3: DrawCircleSector({ cx, cy }, radius, 270 + mouthAngle, 270 - mouthAngle, 0, BLACK); break;
        }
    }


};



// -------------------- Main --------------------
int main() {
    vector<string> mazeLayout = {
 " ################### ",
 " #........#.....O..# ",
 " # ##.###.#.###.## # ",
 " #..O..............# ",
 " #.##.#.#####.#.##.# ",
 " #....#...#...#....# ",
 " ####.### # ###.#### ",
 "    #.#       #.#    ",
 "#####.# ##### #.#####",
 "     .  #   #  .     ",
 "#####.# ##### #.#####",
 "    #.#...O...#.#    ",
 " ####.# ##### #.#### ",
 " #........#........# ",
 " #.##.###.#.###.##.# ",
 " # .#.O.........#. # ",
 " ##.#.#.#####.P.#.## ",
 " #....#...#...#....# ",
 " #.######.#.######.# ",
 " #...............O.# ",
 " ################### "
    };

    array<array<Cell, MAP_HEIGHT>, MAP_WIDTH> dummyMap;

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char c = mazeLayout[y][x];
            if (c == '#') dummyMap[x][y] = Cell::Wall;
            else if (c == 'O') dummyMap[x][y] = Cell::Energizer;
            else if (c == '.') dummyMap[x][y] = Cell::Pellet;
            else dummyMap[x][y] = Cell::Empty;
        }
    }



    int tileSize = 30;
    Map maze(mazeLayout, tileSize);

    int startGX = -1, startGY = -1;
    for (int y = 0; y < maze.rows; y++)
        for (int x = 0; x < maze.cols; x++)
            if (maze.layout[y][x] == 'P') {
                startGX = x; startGY = y; break;
            }

    if (startGX == -1) { startGX = maze.cols / 2; startGY = maze.rows - 2; }

    Pacman pac(startGX * tileSize, startGY * tileSize, tileSize);

    int winW = maze.cols * tileSize;
    int winH = maze.rows * tileSize;
    InitWindow(winW, winH, "PacMaze - Raylib Grid Integrated");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Move Pacman based on arrow keys
        pac.update(maze); // We'll create dummyMap below

        BeginDrawing();
        ClearBackground(BLACK);
        maze.Draw();
        pac.draw();
        DrawText("PACMAZE DEMO", 8, 6, 16, WHITE);
        EndDrawing();
    }


    CloseWindow();
    return 0;
}
