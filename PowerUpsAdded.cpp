
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

// --------------------------------------
// GLOBAL GAME CONSTANTS
// --------------------------------------

// Frightened mode total duration (7 seconds at 60 FPS)
const int FRIGHTENED_TOTAL_FRAMES = 60 * 7;

// Start flashing during the last 2 seconds of frightened mode
const int FRIGHTENED_FLASH_START = FRIGHTENED_TOTAL_FRAMES - (60 * 2);

// Flashing pattern for ghosts (flash every 15 frames)
const int FRIGHTENED_FLASH_INTERVAL = 15;

// Collision radius used for Pacman–Ghost collision
const float PACMAN_GHOST_COLLISION_DIST = 20.0f;

// Tile size used in movement and cage repositioning
const int TILE_SIZE = 20;   // <-- Change this to match your game tile size


// release state machine values (separate from Ghost class)
enum ReleaseState { R_IN_CAGE = 0, R_EXITING_GATE = 1, R_ACTIVE = 2 };

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER
};


// per-ghost release information (kept externally, NOT inside Ghost)
struct ReleaseInfo {
    ReleaseState state;
    int timer;            // optional per-ghost counter (frames)
    bool justEnteredScatter; // if you need this flag like you used earlier
    ReleaseInfo() : state(R_IN_CAGE), timer(0), justEnteredScatter(false) {}
};

int frightenedTimer = 0;        // how many frames left
bool frightenedFlash = false;   // whether ghosts should flash
const int FRIGHTENED_DURATION = 6 * 60;  // 6 seconds
const int FLASH_START = 2 * 60;          // last 2 seconds flash

//Power ups
struct PowerUp {
    int priority;       // bigger = higher priority
    string type;   // "life", "score", "speed"

    bool operator<(const PowerUp& other) const {
        return priority < other.priority; // max-heap
    }
};
priority_queue<PowerUp> pq;

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
    vector<pair<int, int>> mysteryPowerUps;
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
        // Initialize mystery powerups
        mysteryPowerUps = {
             {8,6}, {15,7}, {15,12}
        };

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
                    DrawCircle(cx, cy, tileSize * 0.3f, ORANGE);
                }
		// Mystery Power-Ups
        for (auto& tile : mysteryPowerUps) {
            int cx = tile.first * tileSize + tileSize / 2;
            int cy = tile.second * tileSize + tileSize / 2;

            // Draw background circle
            DrawCircle(cx, cy, tileSize * 0.3f, PURPLE);

            // Draw the "?" symbol in the center
            int fontSize = tileSize / 2;
            DrawText("?", cx - fontSize / 4, cy - fontSize / 2, fontSize, WHITE);
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
public:
    float x, y;
    int tileSize;
    float speed;
    float radius;
    int lives;
    int score;
    int speedTimer;
    bool alive;
    bool dying;          
    int death_timer;
    const int DEATH_FRAMES = 30;
    float startXPos, startYPos;
    bool animation_over;
    unsigned short animation_timer;
    unsigned short energizer_timer;

    // Directions
    enum Direction { LEFT = 0, RIGHT = 1, DOWN = 2, UP = 3 };
    Direction direction;
    Direction desiredDirection;   // CHECK INPUT


    Pacman(int startX, int startY, int tSize)
        : x((float)startX), y((float)startY), tileSize(tSize),
        speed(3.6f), animation_over(false), alive(true),
        direction(RIGHT), desiredDirection(RIGHT), animation_timer(0), energizer_timer(0), lives(3), dying(false), death_timer(0)
    {
        radius = tileSize * 0.3f;
        startXPos = (float)x;
        startYPos = (float)y;
        score = 300;
        speedTimer = 0;
    }

    void resetPosition() {
        // Reset position to start point
        alive = true;
        // Optionally reset x,y
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

    // ------------------------------------------------------
// NEW FUNCTION: Pacman eats a large pellet (O)
// ------------------------------------------------------
    void checkLargePellet(Map& map) {
        int gx = (int)((x + tileSize * 0.5f) / tileSize);
        int gy = (int)((y + tileSize * 0.5f) / tileSize);

        // Check if this tile has a big pellet
        if (gx >= 0 && gx < map.cols && gy >= 0 && gy < map.rows &&
            map.layout[gy][gx] == 'O')  // Still 'O' here, since we haven't eaten it yet
        {
            // Eat the pellet (set to empty)
            map.eatLargePelletAt(gx, gy);

            // Activate frightened mode
            energizer_timer = 7 * 60;  // 7 seconds (consistent with frightened() function)
            frightenedTimer = 0;       // Reset for fresh mode

            // Optional: Add sound effect or score here if desired
            // e.g., PlaySound(eatPelletSound); pac.score += 50;
        }
    }

    void speedBoost(int durationFrames) {
        speed += 1.0f;               // temporary boost
        speedTimer = durationFrames;
    }


    void updatePacMan(Map& map) {
        if (dying) {
            death_timer++;
            if (death_timer >= DEATH_FRAMES) {
                dying = false;
                death_timer = 0;
                // reset Pacman to start position
                x = startXPos;
                y = startYPos;
                direction = RIGHT;
                desiredDirection = RIGHT;
                alive = true;
            }
            return; // skip normal movement while dying
        }

        // ---- normal movement code ----
        int gridX = (int)((x + tileSize / 2) / tileSize);
        int gridY = (int)((y + tileSize / 2) / tileSize);
        float centerX = gridX * tileSize + tileSize / 2.0f;
        float centerY = gridY * tileSize + tileSize / 2.0f;

        if (fabs(x - centerX) < 0.4f) x = centerX;
        if (fabs(y - centerY) < 0.4f) y = centerY;

        // input and movement (desiredDirection & direction)
        if (IsKeyDown(KEY_RIGHT)) desiredDirection = RIGHT;
        if (IsKeyDown(KEY_LEFT))  desiredDirection = LEFT;
        if (IsKeyDown(KEY_UP))    desiredDirection = UP;
        if (IsKeyDown(KEY_DOWN))  desiredDirection = DOWN;

        auto canMove = [&](Pacman::Direction dir) -> bool {
            float tryX = x, tryY = y;
            switch (dir) {
            case LEFT:  tryX -= speed; break;
            case RIGHT: tryX += speed; break;
            case UP:    tryY -= speed; break;
            case DOWN:  tryY += speed; break;
            }
            return !collidesAtCenter(map, tryX + tileSize / 2.0f, tryY + tileSize / 2.0f);
            };

        if (canMove(desiredDirection)) direction = desiredDirection;

        float dx = 0, dy = 0;
        switch (direction) {
        case RIGHT: dx = speed; break;
        case LEFT:  dx = -speed; break;
        case UP:    dy = -speed; break;
        case DOWN:  dy = speed; break;
        }

        if (!collidesAtCenter(map, x + dx + tileSize / 2.0f, y + tileSize / 2.0f)) x += dx;
        if (!collidesAtCenter(map, x + tileSize / 2.0f, y + dy + tileSize / 2.0f)) y += dy;

        int eatGX = (int)((x + tileSize / 2) / tileSize);
        int eatGY = (int)((y + tileSize / 2) / tileSize);

        // Eat small pellets (dots) - this is fine as-is
        map.coins.eatCoinAt(eatGX, eatGY);
        checkLargePellet(map);

        if (energizer_timer > 0) energizer_timer--;
        animation_timer++;

        // ---- handle speed boost timer ----
        if (speedTimer > 0) {
            speedTimer--;
            if (speedTimer == 0) speed = 3.6f; // reset to normal speed
        }
    }

    void draw(bool victory = false) {
        float cx = x + tileSize / 2;
        float cy = y + tileSize / 2;

        if (dying) {
            // death animation: mouth opens wider and shrinks radius
            float t = (float)death_timer / DEATH_FRAMES;
            float deathRadius = radius * (1.0f - 0.5f * t);
            float mouthAngle = 180.0f * t; // opens from 0 -> 180 degrees
            DrawCircle((int)cx, (int)cy, deathRadius, YELLOW);
            DrawCircleSector({ cx, cy }, deathRadius, mouthAngle, -mouthAngle, 0, BLACK);
            return;
        }

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
// Power-Ups function
static void processMysteryPowerUp(Pacman& pac) {
    const int maxLives = 3;

    if (pac.lives < maxLives) pq.push({ 3, "life" }); // life has highest priority
    else {
        pq.push({ 2, "score" }); // only push score if lives full
        // only push speed if score will increase AND lives max
        pq.push({ 1, "speed" });
    }
    
    while (!pq.empty()) {
        PowerUp chosen = pq.top();
        pq.pop();

        if (chosen.type == "life" && pac.lives < maxLives) pac.lives++;
        else if (chosen.type == "score") pac.score += 200;
        else if (chosen.type == "speed") pac.speedBoost(5);
    }
   
}


enum GhostState { NORMAL, FRIGHTENED, EYES };

// -------------------- Ghost --------------------
class Ghost {
public:
    Vector2 position;
    int id;
    int direction;
    int frightened_mode;          // keep your original variable
    int animation_timer;
    Texture2D texture;

    ReleaseState releaseState = R_IN_CAGE;
    float speed = 1.0f;

    int cageX, cageY;             // center of cage
    int gateX, gateY;             // door tile
    int eyesTargetX, eyesTargetY; // tile to move toward when eaten

    Ghost(int gx, int gy, int i_id, Texture2D tex, int tileSize)
        : id(i_id), direction(0), frightened_mode(0),
        animation_timer(0), texture(tex)
    {
        position = { (float)gx * tileSize, (float)gy * tileSize };
        cageX = gx;
        cageY = gy;
        gateX = gx;       // usually gate is above cage
        gateY = gy - 1;   // one tile above
        eyesTargetX = gateX;
        eyesTargetY = gateY;
    }

    // -------------------- NEW: move ghost to gate (eyes mode) --------------------
    void moveToGate(Map& map, int tileSize)
    {
        if (frightened_mode != 2) return; // only eyes mode

        int gx = (int)(position.x / tileSize);
        int gy = (int)(position.y / tileSize);

        // move step by step
        if (gx < eyesTargetX && !map.isWall(gx + 1, gy)) position.x += speed;
        else if (gx > eyesTargetX && !map.isWall(gx - 1, gy)) position.x -= speed;
        else if (gy < eyesTargetY && !map.isWall(gx, gy + 1)) position.y += speed;
        else if (gy > eyesTargetY && !map.isWall(gx, gy - 1)) position.y -= speed;

        // reached gate → reset to normal and start release
        gx = (int)(position.x / tileSize);
        gy = (int)(position.y / tileSize);
        if (gx == eyesTargetX && gy == eyesTargetY)
        {
            frightened_mode = 0;           // back to normal
            releaseState = R_EXITING_GATE; // start normal release
        }
    }

    // -------------------- draw() unchanged except optional eyes handling --------------------
    void draw(bool i_flash, int tileSize) {
        int body_frame = (animation_timer / GHOST_ANIMATION_SPEED) % GHOST_ANIMATION_FRAMES;
        Rectangle srcBody = { body_frame * 16.0f, 0.0f, 16.0f, 16.0f };
        Rectangle srcFace;
        Rectangle dstRect = { position.x, position.y, (float)tileSize, (float)tileSize };
        Vector2 origin = { 0.0f, 0.0f };

        Color bodyColor = WHITE;
        switch (id) {
        case 0: bodyColor = RED; break;
        case 1: bodyColor = Color{ 255,182,255,255 }; break;
        case 2: bodyColor = Color{ 0,255,255,255 }; break;
        case 3: bodyColor = Color{ 255,182,85,255 }; break;
        }

        if (frightened_mode == 0) {
            srcFace = { (float)(CELL_SIZE * direction), (float)CELL_SIZE, (float)CELL_SIZE, (float)CELL_SIZE };
            DrawTexturePro(texture, srcBody, dstRect, origin, 0.0f, bodyColor);
            DrawTexturePro(texture, srcFace, dstRect, origin, 0.0f, WHITE);
        }
        else if (frightened_mode == 1) {
            Color frightenedBlue = Color{ 36,36,255,255 };
            Color faceColor = WHITE;
            srcFace = { (float)(4 * CELL_SIZE), (float)CELL_SIZE, (float)CELL_SIZE, (float)CELL_SIZE };
            if (i_flash && (body_frame % 2) == 0) {
                bodyColor = WHITE;
                faceColor = Color{ 255, 0, 0, 255 };
            }
            else bodyColor = frightenedBlue;

            DrawTexturePro(texture, srcBody, dstRect, origin, 0.0f, bodyColor);
            DrawTexturePro(texture, srcFace, dstRect, origin, 0.0f, faceColor);
        }
        else if (frightened_mode == 2) { // eyes
            srcFace = { (float)(CELL_SIZE * direction), (float)(2 * CELL_SIZE), (float)CELL_SIZE, (float)CELL_SIZE };
            DrawTexturePro(texture, srcFace, dstRect, origin, 0.0f, WHITE);
        }

        animation_timer = (animation_timer + 1) % (GHOST_ANIMATION_FRAMES * GHOST_ANIMATION_SPEED);
    }

    // -------------------- updateReleaseState() minimally changed --------------------
    void updateReleaseState(Map& map, float tileSize)
    {
        if (frightened_mode == 2) {
            moveToGate(map, tileSize); // only eyes mode
            return;
        }

        switch (releaseState)
        {
        case R_IN_CAGE:
            if (position.y > gateY * tileSize) position.y -= speed;
            else releaseState = R_EXITING_GATE;
            break;
        case R_EXITING_GATE:
            if (position.y > (gateY - 1) * tileSize) position.y -= speed;
            else releaseState = R_ACTIVE;
            break;
        case R_ACTIVE:
            break;
        }
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
    navigateToTile(g, map, tileSize, px, py, 2.0f);
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
    navigateToTile(g, map, tileSize, tx, ty, 2.0f);
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

        navigateGhostToTile(*this, m, tileSize, tx, ty, 2.0f);
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

        navigateGhostToTile(*this, m, tileSize, tx, ty, 2.0f);
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
        navigateGhostToTile(*this, m, tileSize, tx, ty, 2.0f);
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
            navigateGhostToTile(*this, m, tileSize, tx, ty, 2.0f);
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
        navigateGhostToTile(*this, m, tileSize, tx, ty, 2.0f);
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

        navigateGhostToTile(*this, m, tileSize, tx, ty, 2.0f);
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

void frightened(
    Pacman& pac,
    RedGhost& red,
    PinkGhost& pink,
    OrangeGhost& orange,
    BlueGhost& blue,
    int& frightenedTimer,   // global timer for this mode
    int tileSize,
    ReleaseInfo& redInfo,
    ReleaseInfo& pinkInfo,
    ReleaseInfo& orangeInfo,
    ReleaseInfo& blueInfo,
    Map& map,
    int& framesSinceStart
)
{
    vector<Ghost*> ghosts = { &red, &pink, &orange, &blue };

    // Increment frightened timer
    frightenedTimer++;

    bool flashing = false;
    if (frightenedTimer >= FRIGHTENED_FLASH_START) {
        // last 2 seconds: flash every 15 frames
        flashing = (frightenedTimer % 30 < 15);
    }

    // Set all ghosts to frightened mode
    for (Ghost* g : ghosts)
    {
        if (g->frightened_mode != 2) // not eaten
        {
            g->frightened_mode = 1; // royal blue
            // Draw function will handle flashing color
        }
    }

    // Handle collisions: Pacman is chaser
    for (Ghost* g : ghosts)
    {
        float dx = fabs(pac.x - g->position.x);
        float dy = fabs(pac.y - g->position.y);

        if (dx < 20 && dy < 20) // collision
        {
            // Send ghost to cage
            g->frightened_mode = 2; // eyes
            g->position.x = g->cageX * tileSize;
            g->position.y = g->cageY * tileSize;

            // Reset release state to exit immediately
            switch (g->id) {
            case 0: redInfo.state = R_ACTIVE; redInfo.timer = 0; break;
            case 1: pinkInfo.state = R_EXITING_GATE; pinkInfo.timer = 0; break;
            case 2: blueInfo.state = R_EXITING_GATE; blueInfo.timer = 0; break;
            case 3: orangeInfo.state = R_EXITING_GATE; orangeInfo.timer = 0; break;
            }

            // Optional: add score for eating ghost
            //pac.score += 200;
        }
    }

    // End frightened mode after 7 seconds
    if (frightenedTimer >= FRIGHTENED_TOTAL_FRAMES)
    {
        frightenedTimer = 0; // reset timer

        for (Ghost* g : ghosts) {
            if (g->frightened_mode != 2) {
                g->frightened_mode = 0; // back to normal chase/scatter
            }
        }
    }
}



void checkPacmanGhostCollision(
    Pacman& pac,
    RedGhost& red,
    PinkGhost& pink,
    OrangeGhost& orange,
    BlueGhost& blue,
    int tileSize,
    ReleaseInfo& redInfo,
    ReleaseInfo& blueInfo,
    ReleaseInfo& pinkInfo,
    ReleaseInfo& orangeInfo,
    Map& map,
    int& framesSinceStart
)
{
    // Local static to reset releaseGhost timing after a death
    static int redLeftFrame_global = -1;

    std::vector<Ghost*> ghosts = { &red, &pink, &orange, &blue };

    for (Ghost* g : ghosts)
    {
        float pacCenterX = pac.x + tileSize / 2.0f;
        float pacCenterY = pac.y + tileSize / 2.0f;
        float ghostCenterX = g->position.x + tileSize / 2.0f;
        float ghostCenterY = g->position.y + tileSize / 2.0f;

        float dx = pacCenterX - ghostCenterX;
        float dy = pacCenterY - ghostCenterY;

        float distSq = dx * dx + dy * dy;

        float pacRadius = pac.radius;
        float ghostRadius = tileSize * 0.40f;
        float collisionDist = pacRadius + ghostRadius;
        float collisionDistSq = collisionDist * collisionDist;

        if (distSq < collisionDistSq)
        {
            // ------------------ IF GHOST IS FRIGHTENED ------------------
            if (g->frightened_mode == 1) // ghost is edible
            {
                g->frightened_mode = 2; // eyes mode
                // Send ghost back to cage
                g->position.x = g->cageX * tileSize;
                g->position.y = g->cageY * tileSize;

                // Reset release state to exit immediately
                switch (g->id) {
                case 0: redInfo.state = R_EXITING_GATE; redInfo.timer = 0; break;
                case 1: pinkInfo.state = R_EXITING_GATE; pinkInfo.timer = 0; break;
                case 2: blueInfo.state = R_EXITING_GATE; blueInfo.timer = 0; break;
                case 3: orangeInfo.state = R_EXITING_GATE; orangeInfo.timer = 0; break;
                }

                // Optional: add score for eating ghost
                //pac.score += 200;

                continue; // skip Pac-Man death
            }

            // ------------------ ELSE PACMAN DIES ------------------
            pac.dying = true;
            pac.death_timer = 0;
            pac.lives--;

            if (pac.lives <= 0)
            {
                cout << "GAME OVER!" << endl;
                CloseWindow();
                exit(0);
            }

            // Reset Pac-Man position
            pac.x = pac.startXPos;
            pac.y = pac.startYPos;
            pac.direction = Pacman::RIGHT;
            pac.alive = true;

            // Reset all ghosts back to cage
            const int RED_CAGE_COL = 10, RED_CAGE_ROW = 8;
            const int PINK_CAGE_COL = 9, PINK_CAGE_ROW = 9;
            const int ORANGE_CAGE_COL = 10, ORANGE_CAGE_ROW = 9;
            const int BLUE_CAGE_COL = 11, BLUE_CAGE_ROW = 9;

            red.position = { (float)(RED_CAGE_COL * tileSize), (float)(RED_CAGE_ROW * tileSize) };
            pink.position = { (float)(PINK_CAGE_COL * tileSize), (float)(PINK_CAGE_ROW * tileSize) };
            orange.position = { (float)(ORANGE_CAGE_COL * tileSize), (float)(ORANGE_CAGE_ROW * tileSize) };
            blue.position = { (float)(BLUE_CAGE_COL * tileSize), (float)(BLUE_CAGE_ROW * tileSize) };

            // Clear frightened state
            red.frightened_mode = 0;
            pink.frightened_mode = 0;
            orange.frightened_mode = 0;
            blue.frightened_mode = 0;

            // Reset release states
            redInfo.state = R_ACTIVE; redInfo.timer = 0; redInfo.justEnteredScatter = false;
            pinkInfo.state = R_IN_CAGE; pinkInfo.timer = 0; pinkInfo.justEnteredScatter = false;
            orangeInfo.state = R_IN_CAGE; orangeInfo.timer = 0; orangeInfo.justEnteredScatter = false;
            blueInfo.state = R_IN_CAGE; blueInfo.timer = 0; blueInfo.justEnteredScatter = false;

            framesSinceStart = 0;
            redLeftFrame_global = -1;

            // Restart release cycle
            releaseGhost(pink, red, map, tileSize, framesSinceStart, pinkInfo);
            releaseGhost(orange, red, map, tileSize, framesSinceStart, orangeInfo);
            releaseGhost(blue, red, map, tileSize, framesSinceStart, blueInfo);

            return; // very important: avoid multi-collision same frame
        }
    }
}



void DrawLives(int lives, int tileSize, int screenWidth) {
    // Position lives at top-right
    float startX = screenWidth - (lives * 35) - 20; // 20px margin from right
    float y = 25;                                   // top margin
    float r = tileSize * 0.25f;                     // mini pacman radius

    for (int i = 0; i < lives; i++) {
        float lx = startX + i * 35;  // spacing between icons
        float ly = y;

        float open = 40; // Mouth opening angle

        // Body
        DrawCircle(lx, ly, r, YELLOW);

        // Mouth - facing RIGHT
        DrawCircleSector({ lx, ly }, r, open, -open, 0, BLACK);
    }
}

// Load this once at the start of your program (before the game loop)

// DrawStartScreen function with button

int frameCounter = 0;
void DrawBlinkingTextFrames(const char* text, Vector2 pos, int fontSize, int spacing, Color color, Font titleFont)
{
    frameCounter++;
    int blinkSpeed = 30; // frames visible (half a second at 60 FPS)

    if ((frameCounter / blinkSpeed) % 2 == 0) {
        DrawTextEx(titleFont, text, pos, (float)fontSize, (float)spacing, color);
    }
}


void DrawStartScreen(int winW, int winH, Font titleFont)
{
    // --- Black background ---
    ClearBackground(BLACK);

    //    // ---------- Title ----------
    const char* title = "PAC-MAZE";
    int titleFontSize = 50;
    int titleSpacing = 4;

    Vector2 titleSize = MeasureTextEx(titleFont, title, (float)titleFontSize, (float)titleSpacing);

    Vector2 titlePos = { (winW - titleSize.x) * 0.5f, 150.0f };
    DrawTextEx(titleFont, title, titlePos, (float)titleFontSize, (float)titleSpacing, YELLOW);

    //    // ---------- Press ENTER ----------
    const char* msg = "Press ENTER to Start";
    int msgFontSize = 20;
    int msgSpacing = 3;

    Vector2 msgSize = MeasureTextEx(titleFont, msg, (float)msgFontSize, (float)msgSpacing);
    Vector2 msgPos = { (winW - msgSize.x) * 0.5f, (float)(winH - 180) };
    DrawBlinkingTextFrames(msg, msgPos, msgFontSize, msgSpacing, WHITE, titleFont);
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
		"#####.# GGGGG #.#####", 
        "     .  #   #  .     ",
        "#####.# GGGGG #.#####",
		"    #.#.......#.#    ", 
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

    // Find Pacman start
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

    // Game state
    GameState currentState = STATE_MENU;



    // Load Texture
    Texture2D ghostTexture = LoadTexture("Ghost16.png");
    if (ghostTexture.id == 0)
        cout << "?? Ghost texture not found! Check path.\n";

    // Ghosts
    RedGhost red(10, 8, 0, ghostTexture, tileSize);
    PinkGhost pink(9, 9, 1, ghostTexture, tileSize);
    OrangeGhost orange(10, 9, 3, ghostTexture, tileSize);
    BlueGhost blue(11, 9, 2, ghostTexture, tileSize);

    ReleaseInfo redRelease;  redRelease.state = R_ACTIVE;
    ReleaseInfo pinkRelease, orangeRelease, blueRelease;

    int globalFrames = 0;
    int waveTimer = 0;
    bool scatterMode = true;
    int frightenedTimer = 0;


    Font titleFont = LoadFont("pacman_font.ttf");  // Replace with your font file



    // -------------------- GAME LOOP --------------------
    while (!WindowShouldClose()) {

        BeginDrawing();

        // -------------------- START MENU --------------------
        if (currentState == STATE_MENU) {

            DrawStartScreen(winW, winH, titleFont);

            if (IsKeyPressed(KEY_ENTER)) {
                currentState = STATE_PLAYING;

                // reset game start timers ONLY ONCE at fresh start
                globalFrames = 0;
                waveTimer = 0;
            }

            EndDrawing();
            continue;   // skip gameplay logic
        }

        // -------------------- GAMEPLAY MODE --------------------
        ClearBackground(BLACK);
        globalFrames++;

        {
            int gx = pac.x / tileSize;
            int gy = pac.y / tileSize;

            if (maze.layout[gy][gx] == 'O')
            {
                maze.layout[gy][gx] = ' '; // eat

                pac.energizer_timer = 10 * 60;
                frightenedTimer = 0;

                red.frightened_mode =
                    pink.frightened_mode =
                    orange.frightened_mode =
                    blue.frightened_mode = 1;
            }
        }

        int pacGridX = (int)(pac.x / pac.tileSize);
        int pacGridY = (int)(pac.y / pac.tileSize);

        for (int i = 0; i < maze.mysteryPowerUps.size(); i++) {
            int mx = maze.mysteryPowerUps[i].first;
            int my = maze.mysteryPowerUps[i].second;

            if (pacGridX == mx && pacGridY == my) {
                processMysteryPowerUp(pac);  // apply PQ logic
                maze.mysteryPowerUps.erase(maze.mysteryPowerUps.begin() + i); // remove eaten tile
                i--; // adjust index
            }
        }

        // Pacman death animation handler
        if (pac.dying) {
            pac.death_timer++;
            if (pac.death_timer > 90) {
                pac.dying = false;
                pac.death_timer = 0;

                // Reset Pac-Man
                pac.x = pac.startXPos;
                pac.y = pac.startYPos;
                pac.direction = Pacman::RIGHT;
                pac.desiredDirection = Pacman::RIGHT;

                // Reset Ghosts
                red.position = { 10.f * tileSize, 8.f * tileSize };
                pink.position = { 9.f * tileSize, 9.f * tileSize };
                orange.position = { 10.f * tileSize, 9.f * tileSize };
                blue.position = { 11.f * tileSize, 9.f * tileSize };

                red.frightened_mode = pink.frightened_mode = orange.frightened_mode = blue.frightened_mode = 0;

                // Ghost release reset
                redRelease.state = R_ACTIVE;
                pinkRelease.state = orangeRelease.state = blueRelease.state = R_IN_CAGE;

                globalFrames = 0;    // FOR RELEASE SYSTEM
            }
        }
        else {
            // Release ghosts
            releaseGhost(pink, red, maze, tileSize, globalFrames, pinkRelease);
            releaseGhost(orange, red, maze, tileSize, globalFrames, orangeRelease);
            releaseGhost(blue, red, maze, tileSize, globalFrames, blueRelease);

            // 1️⃣ Pac-Man moves
            pac.updatePacMan(maze);

            // 2️⃣ Check for large pellet

        // -------------------- FRIGHTENED MODE --------------------
            if (pac.energizer_timer > 0)
            {
                // Decrease energizer timer each frame
                pac.energizer_timer--;

                // Call frightened function
                frightened(
                    pac,
                    red, pink, orange, blue,
                    frightenedTimer,        // this will increment inside frightened()
                    tileSize,
                    redRelease, pinkRelease, orangeRelease, blueRelease,
                    maze,
                    globalFrames
                );
            }
            else
            {
                // Energizer finished → reset frightened mode if needed
                if (frightenedTimer > 0)
                {
                    frightenedTimer = 0;
                    red.frightened_mode = pink.frightened_mode = orange.frightened_mode = blue.frightened_mode = 0;
                }
            }

            // Collision detection
            checkPacmanGhostCollision(
                pac, red, pink, orange, blue,
                tileSize, redRelease, blueRelease, pinkRelease, orangeRelease,
                maze, globalFrames
            );

            // Scatter/Chase waves
            waveTimer++;
            if (scatterMode && waveTimer > 7 * 60) { scatterMode = false; waveTimer = 0; }
            else if (!scatterMode && waveTimer > 20 * 60) { scatterMode = true; waveTimer = 0; }

            // Update ghosts
            red.update(pac, maze, tileSize, scatterMode);
            if (pinkRelease.state == R_ACTIVE)   pink.update(pac, maze, tileSize, scatterMode);
            if (orangeRelease.state == R_ACTIVE) orange.update(pac, maze, tileSize, scatterMode);
            if (blueRelease.state == R_ACTIVE)   blue.update(pac, red, maze, tileSize, scatterMode);
        }

        // ---- Drawing ----
        maze.Draw();
        pac.draw();

        // ⭐ ADDED — proper flashing
        bool flashing = (frightenedTimer > 4 * 60) && (frightenedTimer % 30 < 15);

        red.draw(flashing, tileSize);
        pink.draw(flashing, tileSize);
        orange.draw(flashing, tileSize);
        blue.draw(flashing, tileSize);

        DrawLives(pac.lives, tileSize, winW);
        
        string scoreText = "Score: " + to_string(pac.score);
        DrawText(scoreText.c_str(), 10, 10, 20, WHITE);

        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();

        std::string speedText = "Speed: " + std::to_string((int)(pac.speed * 10) / 10.0f);
        DrawText(speedText.c_str(), 10, screenHeight - 30, 20, GREEN);

        EndDrawing();
    }

    UnloadTexture(ghostTexture);
    CloseWindow();
    return 0;
}
