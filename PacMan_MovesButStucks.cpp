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


// release state machine values (separate from Ghost class)
enum ReleaseState { R_IN_CAGE = 0, R_EXITING_GATE = 1, R_ACTIVE = 2 };

// per-ghost release information (kept externally, NOT inside Ghost)
struct ReleaseInfo {
    ReleaseState state;
    int timer;            // optional per-ghost counter (frames)
    bool justEnteredScatter; // if you need this flag like you used earlier
    ReleaseInfo() : state(R_IN_CAGE), timer(0), justEnteredScatter(false) {}
};


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

    // ✅ adjacency list (DSA structure)
    unordered_map<int, vector<int>> adjList;

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

        buildAdjList();
    }

    // ✅ build adjacency list (only for walkable cells)
    void buildAdjList() {
        int dx[4] = { 1, -1, 0, 0 };
        int dy[4] = { 0, 0, 1, -1 };

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (layout[y][x] == '#') continue;

                int id = y * cols + x;

                for (int i = 0; i < 4; i++) {
                    int nx = x + dx[i];
                    int ny = y + dy[i];
                    if (nx < 0 || ny < 0 || nx >= cols || ny >= rows) continue;
                    if (layout[ny][nx] == '#') continue;

                    int neighbor = ny * cols + nx;
                    adjList[id].push_back(neighbor);
                }
            }
        }

    }

    void Draw() {
        int pad = 0;
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                char c = layout[y][x];
                int px = x * tileSize;
                int py = y * tileSize;
                if (c != '#')
                    DrawRectangle(px, py, tileSize, tileSize, BLACK);
            }
        }

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (layout[y][x] == '#') {
                    int px = x * tileSize;
                    int py = y * tileSize;
                    int w = tileSize;
                    int h = tileSize;
                    if (y == 0 || layout[y - 1][x] != '#')
                        DrawLineEx({ (float)px, (float)py }, { (float)(px + w), (float)py }, 3, DARKBLUE);
                    if (y == rows - 1 || layout[y + 1][x] != '#')
                        DrawLineEx({ (float)px, (float)(py + h) }, { (float)(px + w), (float)(py + h) }, 3, DARKBLUE);
                    if (x == 0 || layout[y][x - 1] != '#')
                        DrawLineEx({ (float)px, (float)py }, { (float)px, (float)(py + h) }, 3, DARKBLUE);
                    if (x == cols - 1 || layout[y][x + 1] != '#')
                        DrawLineEx({ (float)(px + w), (float)py }, { (float)(px + w), (float)(py + h) }, 3, DARKBLUE);
                }
            }
        }

        //THE BELOW COMMENTED LINES ARE USELESS 

        //// ghost home box
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
            DrawRectangle(left, top, w, h, Color{ 15, 15, 15, 255 });
            DrawRectangleLinesEx({ (float)left, (float)top, (float)w, (float)h }, 2, DARKBLUE);
        }

        coins.drawCoins(tileSize);

        for (int y = 0; y < rows; y++)
            for (int x = 0; x < cols; x++)
                if (layout[y][x] == 'O') {
                    int cx = x * tileSize + tileSize / 2;
                    int cy = y * tileSize + tileSize / 2;
                    DrawCircle(cx, cy, tileSize * 0.2f, ORANGE);
                }

        int gateX = 10 * tileSize;
        int gateY = 8 * tileSize;
        int gateWidth = tileSize * 1;
        DrawLineEx({ (float)gateX, (float)gateY }, { (float)(gateX + gateWidth), (float)gateY }, 3.5f, SKYBLUE);
    }

    bool isWall(int gx, int gy) {
        if (gx < 0 || gx >= cols || gy < 0 || gy >= rows) return true;
        char c = layout[gy][gx];
        if (c == '#' || (gy == 9 && gx == 10) || c == 'G') return true;
        return false;
    }

    void eatLargePelletAt(int gx, int gy) {
        if (gx >= 0 && gx < cols && gy >= 0 && gy < rows && layout[gy][gx] == 'O')
            layout[gy][gx] = ' ';
    }
};


// -------------------- Pacman --------------------
class Pacman {
    bool animation_over;
    bool dead;
    unsigned short animation_timer;
    unsigned short energizer_timer;

public:
    float x, y;
    int tileSize;
    float speed;
    float radius;

    // Directions
    enum Direction { LEFT = 0, RIGHT = 1, DOWN = 2, UP = 3 };
    Direction direction;
    Direction desiredDirection;   // CHECK INPUT


    Pacman(int startX, int startY, int tSize)
        : x((float)startX), y((float)startY), tileSize(tSize),
        speed(4.0f), animation_over(false), dead(false),
        direction(RIGHT), desiredDirection(RIGHT), animation_timer(0), energizer_timer(0)
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
        // -----------------------------------------
        // 0. GRID POSITION
        // -----------------------------------------
        int gridX = (int)((x + tileSize / 2) / tileSize);
        int gridY = (int)((y + tileSize / 2) / tileSize);

        float centerX = gridX * tileSize + tileSize / 2.0f;
        float centerY = gridY * tileSize + tileSize / 2.0f;

        const float SNAP_TOLERANCE = 4.0f;

        // -----------------------------------------
        // 1. READ INPUT
        // -----------------------------------------
        if (IsKeyDown(KEY_RIGHT))  desiredDirection = RIGHT;
        else if (IsKeyDown(KEY_LEFT))  desiredDirection = LEFT;
        else if (IsKeyDown(KEY_UP))    desiredDirection = UP;
        else if (IsKeyDown(KEY_DOWN))  desiredDirection = DOWN;

        // -----------------------------------------
        // 2. MOVEMENT CHECK FUNCTION
        // -----------------------------------------
        auto canMove = [&](Direction dir) {
            float testX = x, testY = y;
            switch (dir) {
            case LEFT:  testX -= speed; break;
            case RIGHT: testX += speed; break;
            case UP:    testY -= speed; break;
            case DOWN:  testY += speed; break;
            }
            return !collidesAtCenter(map,
                testX + tileSize / 2.0f,
                testY + tileSize / 2.0f);
            };

        // -----------------------------------------
        // 3. TRY DESIRED DIRECTION FIRST
        // -----------------------------------------
        if (canMove(desiredDirection)) {
            direction = desiredDirection;
        }

        // -----------------------------------------
        // 4. MOVE PAC-MAN
        // -----------------------------------------
        float dx = 0, dy = 0;
        switch (direction) {
        case RIGHT: dx = speed; break;
        case LEFT:  dx = -speed; break;
        case UP:    dy = -speed; break;
        case DOWN:  dy = speed; break;
        }

        // Move only if path is clear
        if (!collidesAtCenter(map, x + dx + tileSize / 2.0f, y + tileSize / 2.0f))
            x += dx;
        if (!collidesAtCenter(map, x + tileSize / 2.0f, y + dy + tileSize / 2.0f))
            y += dy;

        // -----------------------------------------
        // 5. SNAP TO CENTER AFTER MOVEMENT
        // -----------------------------------------
        if (fabs(x - centerX) < SNAP_TOLERANCE) x = centerX;
        if (fabs(y - centerY) < SNAP_TOLERANCE) y = centerY;

        // -----------------------------------------
        // 6. EAT PELLETS
        // -----------------------------------------
        int eatGX = (int)((x + tileSize * 0.5f) / tileSize);
        int eatGY = (int)((y + tileSize * 0.5f) / tileSize);
        map.coins.eatCoinAt(eatGX, eatGY);
        map.eatLargePelletAt(eatGX, eatGY);

        // -----------------------------------------
        // 7. TIMERS
        // -----------------------------------------
        if (energizer_timer > 0) energizer_timer--;
        animation_timer++;
    }



   /* void update(Map& map) {
        int gridX = (int)((x + tileSize / 2) / tileSize);
        int gridY = (int)((y + tileSize / 2) / tileSize);
        float centerX = gridX * tileSize + tileSize / 2.0f;
        float centerY = gridY * tileSize + tileSize / 2.0f;

        if (fabs(x - centerX) < 0.4f) x = centerX;
        if (fabs(y - centerY) < 0.4f) y = centerY;

        float dx = 0, dy = 0;
        if (IsKeyDown(KEY_RIGHT)) { dx += speed; direction = RIGHT; }
        if (IsKeyDown(KEY_LEFT)) { dx -= speed; direction = LEFT; }
        if (IsKeyDown(KEY_UP)) { dy -= speed; direction = UP; }
        if (IsKeyDown(KEY_DOWN)) { dy += speed; direction = DOWN; }

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
*/
    void draw(bool victory = false) {
        float cx = x + tileSize / 2;
        float cy = y + tileSize / 2;

        Color pacColor = victory ? GOLD : YELLOW;
        DrawCircle((int)cx, (int)cy, radius, pacColor);

        float mouthAngle = 40 * sin(animation_timer * 0.15f);
        switch (direction) {
        case LEFT:  DrawCircleSector({ cx, cy }, radius, 180 + mouthAngle, 180 - mouthAngle, 0, BLACK); break;
        case RIGHT: DrawCircleSector({ cx, cy }, radius, mouthAngle, -mouthAngle, 0, BLACK); break;
        case DOWN:  DrawCircleSector({ cx, cy }, radius, 90 + mouthAngle, 90 - mouthAngle, 0, BLACK); break;
        case UP:    DrawCircleSector({ cx, cy }, radius, 270 + mouthAngle, 270 - mouthAngle, 0, BLACK); break;
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

};



// -------------------- Main --------------------
// -------------------- Red Ghost Chase Function --------------------


// -------------------- Global BFS navigator --------------------
// Navigate ghost 'g' toward grid tile (tx,ty) through maze using BFS.
// If exact path exists, follow it; otherwise pick best reachable tile
// closest to (tx,ty). Moves smoothly by 'speed' pixels/frame.
void navigateToTile(Ghost& g, Map& map, int tileSize, int tx, int ty, float speed) {
    // current ghost tile
    int gx = (int)((g.position.x + tileSize / 2) / tileSize);
    int gy = (int)((g.position.y + tileSize / 2) / tileSize);

    // clamp target
    if (tx < 0) tx = 0; if (tx >= map.cols) tx = map.cols - 1;
    if (ty < 0) ty = 0; if (ty >= map.rows) ty = map.rows - 1;

    // BFS setup
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
            if (nx >= 0 && nx < map.cols && ny >= 0 && ny < map.rows &&
                !visited[ny][nx] && !map.isWall(nx, ny))
            {
                visited[ny][nx] = true;
                parent[ny][nx] = cur;
                if (nx == tx && ny == ty) { found = true; break; }
                q.push({ ny, nx });
            }
        }
    }

    // determine next cell to move toward
    pair<int, int> nextCell = { gy, gx };

    if (found) {
        // backtrack from target until the immediate step next to ghost
        pair<int, int> cur = { ty, tx };
        // safety guard in case of malformed parent
        while (parent[cur.first][cur.second] != make_pair(gy, gx)) {
            if (parent[cur.first][cur.second].first == -1) break;
            cur = parent[cur.first][cur.second];
        }
        nextCell = cur;
    }
    else {
        // choose best reachable tile (closest to target by Manhattan) among visited
        int bestDist = INT_MAX;
        pair<int, int> bestCell = { gy, gx };
        for (int r = 0; r < map.rows; ++r) {
            for (int c = 0; c < map.cols; ++c) {
                if (visited[r][c]) {
                    int manh = abs(c - tx) + abs(r - ty);
                    if (manh < bestDist && !(r == gy && c == gx)) {
                        bestDist = manh;
                        bestCell = { r, c };
                    }
                }
            }
        }

        if (!(bestCell.first == gy && bestCell.second == gx)) {
            // backtrack from bestCell to the step next to ghost
            pair<int, int> cur = bestCell;
            while (parent[cur.first][cur.second] != make_pair(gy, gx)) {
                if (parent[cur.first][cur.second].first == -1) break;
                cur = parent[cur.first][cur.second];
            }
            nextCell = cur;
        }
        else {
            // fallback: try any adjacent free tile so ghost doesn't stay stuck
            bool moved = false;
            for (auto& d : dirs) {
                int ny = gy + d[0];
                int nx = gx + d[1];
                if (nx >= 0 && nx < map.cols && ny >= 0 && ny < map.rows && !map.isWall(nx, ny)) {
                    nextCell = { ny, nx };
                    moved = true;
                    break;
                }
            }
            if (!moved) nextCell = { gy, gx }; // no move possible
        }
    }

    // Smooth movement toward nextCell
    float targetX = nextCell.second * tileSize + tileSize / 2.0f;
    float targetY = nextCell.first * tileSize + tileSize / 2.0f;
    float dx = targetX - (g.position.x + tileSize / 2.0f);
    float dy = targetY - (g.position.y + tileSize / 2.0f);
    float len = sqrtf(dx * dx + dy * dy);

    if (len > 0.0001f) {
        dx = dx / len * speed;
        dy = dy / len * speed;
        g.position.x += dx;
        g.position.y += dy;
    }
}

// -------------------- Convenience wrappers --------------------

// chase wrapper: compute pacman tile then navigate to it with chase speed
void chasePacmanToTarget(Ghost& g, Pacman& p, Map& map, int tileSize) {
    // Optionally allow non-red ghosts to call this; caller may decide
    int px = (int)((p.x + tileSize / 2) / tileSize);
    int py = (int)((p.y + tileSize / 2) / tileSize);
    navigateToTile(g, map, tileSize, px, py, 3.0f);
}

// scatter wrapper: find sensible top-right (or other provided) and navigate with scatter speed
void scatterToCorner(Ghost& g, Map& map, int tileSize) {
    // pick a top-right non-wall tile (search top rows from right to left)
    int tx = -1, ty = -1;
    for (int row = 0; row < map.rows; ++row) {
        for (int col = map.cols - 1; col >= 0; --col) {
            if (!map.isWall(col, row)) { tx = col; ty = row; break; }
        }
        if (tx != -1) break;
    }
    if (tx == -1) { tx = map.cols - 2; ty = 1; } // fallback
    navigateToTile(g, map, tileSize, tx, ty, 2.5f);
}

// Generic wrapper if you want to directly call with any tile:
void navigateGhostToTile(Ghost& g, Map& map, int tileSize, int targetCol, int targetRow, float speed) {
    navigateToTile(g, map, tileSize, targetCol, targetRow, speed);
}

// Global function to release a ghost (Pink/Blue/Orange) relative to when RED left the gate.
// DOES NOT require modifying the Ghost class. Uses an external ReleaseInfo for per-ghost state.
// framesSinceStart: integer frame counter (incremented in main loop)
void releaseGhost(
    Ghost& g,
    Ghost& red,
    Map& map,
    int tileSize,
    int framesSinceStart,
    ReleaseInfo& info   // external per-ghost state you must supply
) {
    const int GATE_EXIT_Y = 7;       // gate row as in your code
    const float exitSpeed = 2.0f;    // speed when exiting gate

    // delays measured from the moment RED leaves the gate (frames @ 60 FPS)
    const int DELAY_PINK_FRAMES = 8 * 60;   // Pink: 8s after Red left
    const int DELAY_ORANGE_FRAMES = 13 * 60;   // Orange: 13s after Red left (5s after Pink)
    const int DELAY_BLUE_FRAMES = 16 * 60;   // Blue: 16s after Red left (8s after Pink)

    // static stores when RED first left the cage (set once)
    static int redLeftFrame = -1;
    // detect red leaving gate (first time)
    if (redLeftFrame == -1) {
        if ((int)(red.position.y / tileSize) < GATE_EXIT_Y) {
            redLeftFrame = framesSinceStart;
        }
    }

    // If red hasn't left yet, keep others in cage
    if (redLeftFrame == -1) return;

    // determine target absolute frame for this ghost based on its id
    int targetAbsFrame = INT_MAX;
    if (g.id == 1) targetAbsFrame = redLeftFrame + DELAY_PINK_FRAMES;     // pink
    else if (g.id == 3) targetAbsFrame = redLeftFrame + DELAY_ORANGE_FRAMES; // orange
    else if (g.id == 2) targetAbsFrame = redLeftFrame + DELAY_BLUE_FRAMES;   // blue
    else return; // non-managed ghost (e.g., red), just return

    // State machine using external ReleaseInfo
    switch (info.state) {
    case R_IN_CAGE:
        // Wait until global frames reach the target
        if (framesSinceStart >= targetAbsFrame) {
            info.state = R_EXITING_GATE;
            info.timer = 0;
        }
        break;

    case R_EXITING_GATE:
        // Move ghost upwards out of the gate
        g.position.y -= exitSpeed;

        // When ghost's grid row <= GATE_EXIT_Y, mark ACTIVE and snap to center
        if ((int)(g.position.y / tileSize) <= GATE_EXIT_Y) {
            info.state = R_ACTIVE;

            // Snap to tile center to avoid jitter
            g.position.x = ((int)(g.position.x / tileSize) * tileSize) + tileSize / 2.0f;
            g.position.y = ((int)(g.position.y / tileSize) * tileSize) + tileSize / 2.0f;

            info.timer = 0;
            info.justEnteredScatter = true; // optional: mimic previous behavior
        }
        break;

    case R_ACTIVE:
        // nothing here; ghost's normal update (chase/scatter) should run in its update() only when ACTIVE
        break;
    }
}


// -------------------- RedGhost class (calls the global functions) --------------------
class RedGhost : public Ghost {
public:
    RedGhost(int gx, int gy, int id, Texture2D tex, int tile)
        : Ghost(gx, gy, id, tex, tile) {
    }

    // update will call either chase or scatter behavior using global functions
    void update(Pacman& p, Map& m, int tileSize, bool scatterMode) {
        if (scatterMode) {
            // Use generic scatter wrapper (which chooses top-right)
            scatterToCorner(*this, m, tileSize);
        }
        else {
            // Chase pacman
            chasePacmanToTarget(*this, p, m, tileSize);
        }
    }

    // keep draw method usage compatible with your existing code
    void draw(bool debug, int tileSize) {
        Ghost::draw(debug, tileSize);
    }
};

class PinkGhost : public Ghost {
public:
    PinkGhost(int gx, int gy, int id, Texture2D tex, int tile)
        : Ghost(gx, gy, id, tex, tile) {
    }

    void scatterToTopLeft(Map& m, int tileSize) {
        int tx = -1, ty = -1;

        // find top-left walkable tile
        for (int row = 0; row < m.rows; row++) {
            for (int col = 0; col < m.cols; col++) {
                if (!m.isWall(col, row)) {
                    tx = col;
                    ty = row;
                    break;
                }
            }
            if (tx != -1) break;
        }

        if (tx == -1) { tx = 1; ty = 1; } // fallback

        navigateGhostToTile(*this, m, tileSize, tx, ty, 2.5f);
    }

    void chaseTarget(Pacman& p, Map& m, int tileSize) {
        int tx = (int)((p.x + tileSize / 2) / tileSize);
        int ty = (int)((p.y + tileSize / 2) / tileSize);

        switch (p.direction) {
        case Pacman::UP:    ty -= 4; break;
        case Pacman::DOWN:  ty += 4; break;
        case Pacman::LEFT:  tx -= 4; break;
        case Pacman::RIGHT: tx += 4; break;
        default: break;
        }

        navigateGhostToTile(*this, m, tileSize, tx, ty, 3.0f);
    }

    void update(Pacman& p, Map& m, int tileSize, bool scatterMode) {
        if (scatterMode)
            scatterToTopLeft(m, tileSize);
        else
            chaseTarget(p, m, tileSize);
    }

    void draw(bool debug, int tileSize) {
        Ghost::draw(debug, tileSize);
    }
};

class OrangeGhost : public Ghost {
public:
    OrangeGhost(int gx, int gy, int id, Texture2D tex, int tile)
        : Ghost(gx, gy, id, tex, tile) {
    }

    // Scatter to bottom-left walkable tile
    void scatterToBottomLeft(Map& m, int tileSize) {
        int tx = -1, ty = -1;

        // search bottom-left: start from last row upwards, leftmost columns
        for (int row = m.rows - 1; row >= 0; row--) {
            for (int col = 0; col < m.cols; col++) {
                if (!m.isWall(col, row)) {
                    tx = col;
                    ty = row;
                    break;
                }
            }
            if (tx != -1) break;
        }

        if (tx == -1) { tx = 1; ty = m.rows - 2; } // fallback
        navigateGhostToTile(*this, m, tileSize, tx, ty, 2.5f);
    }

    // Chase 2 tiles in front of Pacman
    void chaseTarget(Pacman& p, Map& m, int tileSize) {
        int tx = (int)((p.x + tileSize / 2) / tileSize);
        int ty = (int)((p.y + tileSize / 2) / tileSize);

        // 2 tiles in front
        switch (p.direction) {
        case Pacman::UP:    ty -= 2; break;
        case Pacman::DOWN:  ty += 2; break;
        case Pacman::LEFT:  tx -= 2; break;
        case Pacman::RIGHT: tx += 2; break;
        default: break;
        }

        // Check distance to pacman (Manhattan)
        int gx = (int)((position.x + tileSize / 2) / tileSize);
        int gy = (int)((position.y + tileSize / 2) / tileSize);
        int manhDist = abs(gx - tx) + abs(gy - ty);

        if (manhDist <= 3) {
            // too close → switch to scatter temporarily
            scatterToBottomLeft(m, tileSize);
        }
        else {
            navigateGhostToTile(*this, m, tileSize, tx, ty, 3.0f);
        }
    }

    void update(Pacman& p, Map& m, int tileSize, bool scatterMode) {
        if (scatterMode)
            scatterToBottomLeft(m, tileSize);
        else
            chaseTarget(p, m, tileSize);
    }

    void draw(bool debug, int tileSize) {
        Ghost::draw(debug, tileSize);
    }
};


class BlueGhost : public Ghost {
public:
    BlueGhost(int gx, int gy, int id, Texture2D tex, int tile)
        : Ghost(gx, gy, id, tex, tile) {
    }

    // Scatter to bottom-right corner
    void scatterToBottomRight(Map& m, int tileSize) {
        int tx = -1, ty = -1;

        for (int row = m.rows - 1; row >= 0; --row) {
            for (int col = m.cols - 1; col >= 0; --col) {
                if (!m.isWall(col, row)) {
                    tx = col;
                    ty = row;
                    break;
                }
            }
            if (tx != -1) break;
        }

        if (tx == -1) { tx = m.cols - 2; ty = m.rows - 2; } // fallback
        navigateGhostToTile(*this, m, tileSize, tx, ty, 2.5f);
    }

    // Chase target using RedGhost and Pacman
    void chaseTarget(Pacman& p, RedGhost& red, Map& m, int tileSize) {
        // Current tile positions
        float pacX = p.x + tileSize / 2.0f;
        float pacY = p.y + tileSize / 2.0f;
        float redX = red.position.x + tileSize / 2.0f;
        float redY = red.position.y + tileSize / 2.0f;

        // Vector from Red → Pacman
        float vx = pacX - redX;
        float vy = pacY - redY;

        // Extend vector to grid boundary
        float scaleX = vx > 0 ? (m.cols * tileSize - pacX) / vx : (pacX) / -vx;
        float scaleY = vy > 0 ? (m.rows * tileSize - pacY) / vy : (pacY) / -vy;
        float scale = std::min(scaleX, scaleY); // scale until hitting boundary

        float targetX = pacX + vx * scale;
        float targetY = pacY + vy * scale;

        // Convert to tile coordinates
        int tx = (int)(targetX / tileSize);
        int ty = (int)(targetY / tileSize);

        navigateGhostToTile(*this, m, tileSize, tx, ty, 3.0f);
    }

    void update(Pacman& p, RedGhost& red, Map& m, int tileSize, bool scatterMode) {
        if (scatterMode)
            scatterToBottomRight(m, tileSize);
        else
            chaseTarget(p, red, m, tileSize);
    }

    void draw(bool debug, int tileSize) {
        Ghost::draw(debug, tileSize);
    }
};


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
        "#####.# GGGGG #.#####",
        "     .  #   #  .     ",
        "#####.# GGGGG #.#####",
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

    // find pacman start
    int startGX = -1, startGY = -1;
    for (int y = 0; y < maze.rows; y++) {
        for (int x = 0; x < maze.cols; x++) {
            if (maze.layout[y][x] == 'P') { startGX = x; startGY = y; break; }
        }
        if (startGX != -1) break;
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
    RedGhost red(10, 8, 0, ghostTexture, tileSize); // Blinky (red)
    PinkGhost pink(9, 9, 1, ghostTexture, tileSize);
    OrangeGhost orange(10, 9, 3, ghostTexture, tileSize);
    BlueGhost blue(11, 9, 2, ghostTexture, tileSize); // ID 2 for blue

    ReleaseInfo pinkRelease;
    ReleaseInfo orangeRelease;
    ReleaseInfo blueRelease;
    int globalFrames = 0;

    int waveTimer = 0;
    bool scatterMode = true; // start scattering

    // -------------------- Game Loop --------------------
    while (!WindowShouldClose()) {
        globalFrames++;

        // Release ghosts
        releaseGhost(pink, red, maze, tileSize, globalFrames, pinkRelease);
        releaseGhost(orange, red, maze, tileSize, globalFrames, orangeRelease);
        releaseGhost(blue, red, maze, tileSize, globalFrames, blueRelease);

        // Update Pacman
        pac.update(maze);

        // Wave timer: scatter 7s, chase 20s
        waveTimer++;
        if (scatterMode && waveTimer > 7 * 60) {
            scatterMode = false;
            waveTimer = 0;
        }
        else if (!scatterMode && waveTimer > 20 * 60) {
            scatterMode = true;
            waveTimer = 0;
        }

        // Update ghosts
        red.update(pac, maze, tileSize, scatterMode);

        if (pinkRelease.state == R_ACTIVE)
            pink.update(pac, maze, tileSize, scatterMode);

        if (orangeRelease.state == R_ACTIVE)
            orange.update(pac, maze, tileSize, scatterMode);

        if (blueRelease.state == R_ACTIVE)
            blue.update(pac, red, maze, tileSize, scatterMode);  // <-- call BlueGhost.update

        // -------------------- Draw --------------------
        BeginDrawing();
        ClearBackground(BLACK);

        maze.Draw();
        pac.draw();

        red.draw(false, tileSize);
        pink.draw(false, tileSize);
        orange.draw(false, tileSize);
        blue.draw(false, tileSize);

        DrawText("PACMAZE DEMO", 8, 6, 16, WHITE);
        EndDrawing();
    }

    UnloadTexture(ghostTexture);
    CloseWindow();
    return 0;
} // <-- final closing brace for main()
