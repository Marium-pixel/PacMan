#include "raylib.h"
#include <iostream>
#include <string>
using namespace std;

// Maze configuration
const int rows = 21;
const int cols = 19;
const int tileSize = 24;

// Pac-Man structure
struct Pacman {
    int x, y;        // Grid position
    float px, py;    // Pixel position for smooth movement
    float speed;
};

// Maze layout (based on original Pac-Man pattern)
const char mazeLayout[rows][cols + 1] = {
    "###################",
    "#........#........#",
    "#.###.###.#.###.###",
    "#o###.###.#.###.###",
    "#.................#",
    "#.###.#.#####.#.###",
    "#.....#...#...#...#",
    "#####.### # ###.###",
    "    #.#       #.#  ",
    "#####.# ## ## #.###",
    "     .  #GG#  .    ",
    "#####.# ##### #.###",
    "    #.#       #.#  ",
    "#####.# ##### #.###",
    "#........#........#",
    "#.###.###.#.###.###",
    "#o..#........#....#",
    "###.#.#.#####.#.###",
    "#.....#...#...#...#",
    "#.#########.#.###.#",
    "#.................#"
};

// Check if a cell is walkable (not a wall)
bool IsWalkable(int x, int y) {
    if (x < 0 || y < 0 || x >= cols || y >= rows) return false;
    return mazeLayout[y][x] != '#';
}

int main() {
    InitWindow(cols * tileSize, rows * tileSize, "PAC-MAN Test");
    SetTargetFPS(60);

    Pacman pacman;
    pacman.x = 9; // starting grid
    pacman.y = 15;
    pacman.px = pacman.x * tileSize + tileSize / 2;
    pacman.py = pacman.y * tileSize + tileSize / 2;
    pacman.speed = 2.0f;

    while (!WindowShouldClose()) {
        // --- Handle input ---
        int nextX = pacman.x;
        int nextY = pacman.y;

        if (IsKeyDown(KEY_RIGHT)) nextX++;
        if (IsKeyDown(KEY_LEFT)) nextX--;
        if (IsKeyDown(KEY_UP)) nextY--;
        if (IsKeyDown(KEY_DOWN)) nextY++;

        // Move if walkable
        if (IsWalkable(nextX, nextY)) {
            pacman.x = nextX;
            pacman.y = nextY;
        }

        // Smooth pixel position
        float targetX = pacman.x * tileSize + tileSize / 2;
        float targetY = pacman.y * tileSize + tileSize / 2;
        pacman.px += (targetX - pacman.px) * 0.2f;
        pacman.py += (targetY - pacman.py) * 0.2f;

        // --- Draw everything ---
        BeginDrawing();
        ClearBackground(BLACK);

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                char cell = mazeLayout[y][x];
                int px = x * tileSize;
                int py = y * tileSize;

                if (cell == '#') {
                    // Dark blue walls
                    DrawRectangle(px, py, tileSize, tileSize, DARKBLUE);
                }
                else if (cell == 'G') {
                    DrawRectangle(px, py, tileSize, tileSize, RED);
                }
                else if (cell == '.' || cell == 'o') {
                    // Dots and power pellets
                    DrawCircle(px + tileSize / 2, py + tileSize / 2,
                        (cell == 'o' ? 5 : 2), YELLOW);
                }
            }
        }

        // Draw Pac-Man
        DrawCircle(pacman.px, pacman.py, tileSize / 2 - 2, YELLOW);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
