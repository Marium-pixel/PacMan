#include "raylib.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <array>
#include <cmath>
#include <queue>
 
using namespace std;

constexpr unsigned char CELL_SIZE = 16;
constexpr unsigned char MAP_HEIGHT = 21;
constexpr unsigned char MAP_WIDTH = 21;

constexpr unsigned char GHOST_ANIMATION_FRAMES = 2;
constexpr unsigned char GHOST_ANIMATION_SPEED = 8;

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
            DrawCircle(cx, cy, tileSize * 0.12f, ORANGE);
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

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (layout[y][x] == '.') coins.addCoin(x, y);
            }
        }
    }

    void Draw() {
        int pad = 0;

        // Draw corridors
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                char c = layout[y][x];
                int px = x * tileSize;
                int py = y * tileSize;
                if (c != '#')
                    DrawRectangle(px, py, tileSize, tileSize, BLACK);
            }
        }

        // Draw walls
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

        // Draw pellets
        coins.drawCoins(tileSize);

        // Draw large pellets
        for (int y = 0; y < rows; y++)
            for (int x = 0; x < cols; x++)
                if (layout[y][x] == 'O') {
                    int cx = x * tileSize + tileSize / 2;
                    int cy = y * tileSize + tileSize / 2;
                    DrawCircle(cx, cy, tileSize * 0.2f, ORANGE);
                }

        // Draw Ghost gate
        int gateX = 10 * tileSize;
        int gateY = 8 * tileSize;
        int gateWidth = tileSize * 1;
        DrawLineEx({ (float)gateX,(float)gateY },
            { (float)(gateX + gateWidth),(float)gateY }, 3.5f, SKYBLUE);
    }

    bool isWall(int gx, int gy) {
        if (gx < 0 || gx >= cols || gy < 0 || gy >= rows) return true;
        char c = layout[gy][gx];
        if (c == '#' || (gy == 9 && gx == 10)) return true;
        return false;
    }

    void eatLargePelletAt(int gx, int gy) {
        if (gx >= 0 && gx < cols && gy >= 0 && gy < rows &&
            layout[gy][gx] == 'O')
            layout[gy][gx] = ' ';
    }


};

// -------------------- Pacman --------------------
class Pacman {
    bool animation_over;
    bool dead;
    unsigned char direction;
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
        radius = tileSize * 0.3f;
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

    void update(Map& map) {
        int gridX = (int)((x + tileSize / 2) / tileSize);
        int gridY = (int)((y + tileSize / 2) / tileSize);
        float centerX = gridX * tileSize + tileSize / 2.0f;
        float centerY = gridY * tileSize + tileSize / 2.0f;

        if (fabs(x - centerX) < 1.5f) x = centerX;
        if (fabs(y - centerY) < 1.5f) y = centerY;

        float dx = 0, dy = 0;
        if (IsKeyDown(KEY_RIGHT)) { dx += speed; direction = 1; }
        if (IsKeyDown(KEY_LEFT)) { dx -= speed; direction = 0; }
        if (IsKeyDown(KEY_UP)) { dy -= speed; direction = 3; }
        if (IsKeyDown(KEY_DOWN)) { dy += speed; direction = 2; }

        if (dx != 0 && dy != 0) {
            float inv = 1.0f / sqrtf(dx * dx + dy * dy);
            dx *= inv * speed;
            dy *= inv * speed;
        }

        float tryX = x + dx;
        float tryY = y + dy;

        if (!collidesAtCenter(map, tryX + tileSize / 2.0f, y + tileSize / 2.0f)) x = tryX;
        if (!collidesAtCenter(map, x + tileSize / 2.0f, tryY + tileSize / 2.0f)) y = tryY;

        int eatGX = (int)((x + tileSize * 0.5f) / tileSize);
        int eatGY = (int)((y + tileSize * 0.5f) / tileSize);
        map.coins.eatCoinAt(eatGX, eatGY);
        map.eatLargePelletAt(eatGX, eatGY);

        if (energizer_timer > 0) energizer_timer--;
        animation_timer++;
    }

    void draw(bool victory = false) {
        float cx = x + tileSize / 2;
        float cy = y + tileSize / 2;

        Color pacColor = victory ? GOLD : YELLOW;
        DrawCircle((int)cx, (int)cy, radius, pacColor);

        float mouthAngle = 40 * sin(animation_timer * 0.15f);
        switch (direction) {
        case 0: DrawCircleSector({ cx, cy }, radius, 180 + mouthAngle, 180 - mouthAngle, 0, BLACK); break;
        case 1: DrawCircleSector({ cx, cy }, radius, mouthAngle, -mouthAngle, 0, BLACK); break;
        case 2: DrawCircleSector({ cx, cy }, radius, 90 + mouthAngle, 90 - mouthAngle, 0, BLACK); break;
        case 3: DrawCircleSector({ cx, cy }, radius, 270 + mouthAngle, 270 - mouthAngle, 0, BLACK); break;
        }
    }
};

enum GhostState { NORMAL, FRIGHTENED, EYES };
// -------------------- Ghost --------------------
class Ghost {
public:
    Vector2 position;
    int id;
    int direction;
    int frightened_mode;
    int animation_timer;
    Texture2D texture;

    Ghost(int gx, int gy, int i_id, Texture2D tex, int tileSize)
        : id(i_id), direction(0), frightened_mode(0),
        animation_timer(0), texture(tex)
    {
        position = { (float)gx * tileSize, (float)gy * tileSize };
    }

    void draw(bool i_flash, int tileSize) {
        // Equivalent to the SFML body_frame calculation
        int body_frame = (animation_timer / GHOST_ANIMATION_SPEED) % GHOST_ANIMATION_FRAMES;

        // Source rectangles assume a sprite sheet arranged like:
        // row 0 = body animation frames (cols 0..GHOST_ANIMATION_FRAMES-1)
        // row 1 = faces (direction frames at cols 0..3, frightened-face at col 4)
        // row 2 = eyes (direction frames at cols 0..3)
        const int SPRITE_SIZE = 16; // same as CELL_SIZE in your SFML source
        Rectangle srcBody = { body_frame * SPRITE_SIZE, 0.0f, (float)SPRITE_SIZE, (float)SPRITE_SIZE };
        Rectangle srcFace; // will be set per state
        Rectangle dstRect = { position.x, position.y, (float)tileSize, (float)tileSize };
        Vector2 origin = { 0.0f, 0.0f };

        // Default body tint (will be overwritten for frightened/flash)
        Color bodyColor = WHITE;
        switch (id) {
        case 0: bodyColor = RED; break;
        case 1: bodyColor = Color{ 255,182,255,255 }; break; // pink-ish
        case 2: bodyColor = Color{ 0,255,255,255 }; break;   // cyan
        case 3: bodyColor = Color{ 255,182,85,255 }; break;  // orange-like (if you later add a 4th ghost)
        }

        // Handle states
        if (frightened_mode == 0) {
            // Normal: draw body + face (face depends on direction)
            srcFace = { (float)(CELL_SIZE * direction), (float)CELL_SIZE, (float)CELL_SIZE, (float)CELL_SIZE };
            // Draw body and face
            DrawTexturePro(texture, srcBody, dstRect, origin, 0.0f, bodyColor);
            DrawTexturePro(texture, srcFace, dstRect, origin, 0.0f, WHITE);
        }
        else if (frightened_mode == 1) {
            // Frightened: blue body, face uses frightened tile (col 4, row 1)
            Color frightenedBlue = Color{ 36,36,255,255 };
            Color faceColor = WHITE;
            srcFace = { (float)(4 * CELL_SIZE), (float)CELL_SIZE, (float)CELL_SIZE, (float)CELL_SIZE };

            // Flashing logic: if i_flash and alternate frame, invert colors like SFML code
            if (i_flash && (body_frame % 2) == 0) {
                bodyColor = WHITE;
                faceColor = Color{ 255, 0, 0, 255 }; // red face when flashing
            }
            else {
                bodyColor = frightenedBlue;
                faceColor = WHITE;
            }

            DrawTexturePro(texture, srcBody, dstRect, origin, 0.0f, bodyColor);
            DrawTexturePro(texture, srcFace, dstRect, origin, 0.0f, faceColor);
        }
        else {
            // EYES (ghost has been eaten) -> only draw eyes (row 2, direction column)
            srcFace = { (float)(CELL_SIZE * direction), (float)(2 * CELL_SIZE), (float)CELL_SIZE, (float)CELL_SIZE };
            DrawTexturePro(texture, srcFace, dstRect, origin, 0.0f, WHITE);
        }

        // Advance animation timer (wrap to avoid overflow)
        animation_timer = (animation_timer + 1) % (GHOST_ANIMATION_FRAMES * GHOST_ANIMATION_SPEED);
    }

    /*void draw(bool i_flash, int tileSize) {
        int frame = (animation_timer / GHOST_ANIMATION_SPEED) % GHOST_ANIMATION_FRAMES;
        Rectangle srcRectBody = { frame * 16.0f, 0, 16, 16 };
        Rectangle dstRect = { position.x, position.y, (float)tileSize, (float)tileSize };

        Color bodyColor = WHITE;
        switch (id) {
        case 0: bodyColor = RED; break;
        case 1: bodyColor = PINK; break;
        case 2: bodyColor = SKYBLUE; break;
        }

        DrawTexturePro(texture, srcRectBody, dstRect, { 0, 0 }, 0.0f, bodyColor);
        animation_timer = (animation_timer + 1) % (GHOST_ANIMATION_FRAMES * GHOST_ANIMATION_SPEED);
    }*/

};



// -------------------- Main --------------------
// -------------------- Red Ghost Chase Function --------------------
void chasePacman(Ghost& g, Pacman& p, Map& map, int tileSize) {
    if (g.id != 0) return; // Only red ghost chases

    int gx = (int)((g.position.x + tileSize / 2) / tileSize);
    int gy = (int)((g.position.y + tileSize / 2) / tileSize);

    int px = (int)((p.x + tileSize / 2) / tileSize);
    int py = (int)((p.y + tileSize / 2) / tileSize);

    // BFS to find shortest path
    vector<vector<bool>> visited(map.rows, vector<bool>(map.cols, false));
    vector<vector<pair<int, int>>> parent(map.rows, vector<pair<int, int>>(map.cols, { -1,-1 }));
    queue<pair<int, int>> q;
    q.push({ gy, gx });
    visited[gy][gx] = true;

    int dirs[4][2] = { {-1,0},{1,0},{0,-1},{0,1} }; // up, down, left, right
    bool found = false;

    while (!q.empty() && !found) {
        auto cur = q.front(); q.pop();
        for (auto& d : dirs) {
            int ny = cur.first + d[0];
            int nx = cur.second + d[1];
            if (nx >= 0 && nx < map.cols && ny >= 0 && ny < map.rows && !visited[ny][nx] && !map.isWall(nx, ny)) {
                visited[ny][nx] = true;
                parent[ny][nx] = cur;
                if (nx == px && ny == py) { found = true; break; }
                q.push({ ny,nx });
            }
        }
    }

    // trace next cell along shortest path
    pair<int, int> nextCell = { gy,gx };
    if (found) {
        pair<int, int> cur = { py,px };
        while (parent[cur.first][cur.second] != make_pair(gy, gx)) {
            cur = parent[cur.first][cur.second];
        }
        nextCell = cur;
    }

    // move smoothly toward next cell
    float targetX = nextCell.second * tileSize + tileSize / 2.0f;
    float targetY = nextCell.first * tileSize + tileSize / 2.0f;
    float dx = targetX - (g.position.x + tileSize / 2.0f);
    float dy = targetY - (g.position.y + tileSize / 2.0f);
    float len = sqrt(dx * dx + dy * dy);
    float speed = 3.0f; // red ghost speed
    if (len > 0) {
        dx = dx / len * speed;
        dy = dy / len * speed;
        g.position.x += dx;
        g.position.y += dy;
    }
}

// -------------------- Main --------------------
int main() {
    vector<string> mazeLayout = {
 " ################### ",
 " #........#.....O..# ",
 " #.##.###.#.###.##.# ",
 " #..O..............# ",
 " #.##.#.#####.#.##.# ",
 " #....#...#...#....# ",
 " ####.### # ###.#### ",
 "    #.#       #.#    ",
 "#####.# ##### #.#####",
 "     .  #   #  .     ",
 "#####.# ##### #.#####",
 "    #.#...O...#.#    ",
 " ####.#.#####.#.#### ",
 " #........#........# ",
 " #.##.###.#.###.##.# ",
 " #..#.O.........#..# ",
 " ##.#.#.#####.P.#.## ",
 " #....#...#...#....# ",
 " #.######.#.######.# ",
 " #...............O.# ",
 " ################### "
    };

    int tileSize = 45;
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

    Texture2D ghostTexture = LoadTexture("Ghost16.png");
    if (ghostTexture.id == 0)
        cout << "?? Ghost texture not found! Check path.\n";

    // -------------------- Ghosts --------------------
    vector<Ghost> ghosts;
    ghosts.push_back(Ghost(10, 8, 0, ghostTexture, tileSize));//red 
    ghosts.push_back(Ghost(9, 9, 1, ghostTexture, tileSize));//pink
    ghosts.push_back(Ghost(11, 9, 2, ghostTexture, tileSize));//blue
    ghosts.push_back(Ghost(10, 9, 3, ghostTexture, tileSize)); // orange 

    float ghostSpeed = 3.0f; // slightly faster for smooth movement

    // Wave timer
    int waveTimer = 0;
    bool scatterMode = true; // start with scatter

    // -------------------- BFS pathfinding for red ghost --------------------
    auto chasePacman = [&](Ghost& red) {
        int gx = (int)((red.position.x + tileSize / 2) / tileSize);
        int gy = (int)((red.position.y + tileSize / 2) / tileSize);

        int px = (int)((pac.x + tileSize / 2) / tileSize);
        int py = (int)((pac.y + tileSize / 2) / tileSize);

        // BFS setup
        vector<vector<bool>> visited(maze.rows, vector<bool>(maze.cols, false));
        vector<vector<pair<int, int>>> parent(maze.rows, vector<pair<int, int>>(maze.cols, { -1,-1 }));
        queue<pair<int, int>> q;
        q.push({ gy,gx });
        visited[gy][gx] = true;
        int dirs[4][2] = { {-1,0},{1,0},{0,-1},{0,1} }; // up, down, left, right

        bool found = false;
        while (!q.empty() && !found) {
            auto cur = q.front(); q.pop();
            for (auto& d : dirs) {
                int ny = cur.first + d[0];
                int nx = cur.second + d[1];
                if (nx >= 0 && nx < maze.cols && ny >= 0 && ny < maze.rows && !visited[ny][nx] && !maze.isWall(nx, ny)) {
                    visited[ny][nx] = true;
                    parent[ny][nx] = cur;
                    if (nx == px && ny == py) { found = true; break; }
                    q.push({ ny,nx });
                }
            }
        }

        // trace back the path
        pair<int, int> nextCell = { gy,gx };
        if (found) {
            pair<int, int> cur = { py,px };
            while (parent[cur.first][cur.second] != make_pair(gy, gx)) {
                cur = parent[cur.first][cur.second];
            }
            nextCell = cur;
        }

        // move smoothly toward next cell
        float targetX = nextCell.second * tileSize + tileSize / 2.0f;
        float targetY = nextCell.first * tileSize + tileSize / 2.0f;
        float dx = targetX - (red.position.x + tileSize / 2);
        float dy = targetY - (red.position.y + tileSize / 2);
        float len = sqrt(dx * dx + dy * dy);
        if (len > 0) {
            dx = dx / len * ghostSpeed;
            dy = dy / len * ghostSpeed;
            red.position.x += dx;
            red.position.y += dy;
        }
        };

    // -------------------- Game Loop --------------------
    while (!WindowShouldClose()) {
        pac.update(maze);

        // -------------------- Wave timer --------------------
        waveTimer++;
        if (scatterMode && waveTimer > 7 * 60) { // scatter 7 sec
            scatterMode = false;
            waveTimer = 0;
        }
        else if (!scatterMode && waveTimer > 20 * 60) { // chase 20 sec
            scatterMode = true;
            waveTimer = 0;
        }

        // -------------------- Update Red Ghost --------------------
        if (scatterMode) {
            // scatter to top-right corner smoothly
            Ghost& red = ghosts[0];
            float targetX = (maze.cols - 2) * tileSize + tileSize / 2.0f;
            float targetY = 1 * tileSize + tileSize / 2.0f;
            float dx = targetX - (red.position.x + tileSize / 2);
            float dy = targetY - (red.position.y + tileSize / 2);
            float len = sqrt(dx * dx + dy * dy);
            if (len > 0) {
                dx = dx / len * ghostSpeed;
                dy = dy / len * ghostSpeed;
                red.position.x += dx;
                red.position.y += dy;
            }
        }
        else {
            chasePacman(ghosts[0]);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        maze.Draw();
        pac.draw();

        for (auto& g : ghosts) {
            g.draw(false, tileSize);
        }

        DrawText("PACMAZE DEMO", 8, 6, 16, WHITE);
        EndDrawing();
    }

    UnloadTexture(ghostTexture);
    CloseWindow();
    return 0;
}
