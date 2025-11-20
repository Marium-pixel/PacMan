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

typedef enum { MENU_PLAY, MENU_HOW_TO, MENU_HIGHSCORE, MENU_EXIT } MenuOption;

typedef enum { STATE_MENU, STATE_LOADING, STATE_ENTER_NAME, STATE_PLAYING, STATE_HIGHSCORE, STATE_HOW_TO, STATE_EXIT } GameState;  // Added STATE_ENTER_NAME


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

string playerName = "";  // Store the player's entered name

// -------------------- Linked List for Pellets --------------------
struct CoinNode {
    int x, y;
    CoinNode* next;
};

class Pacman;

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

    bool eatCoinAt(int gridX, int gridY) {
        CoinNode* curr = head;
        CoinNode* prev = nullptr;

        while (curr) {
            if (curr->x == gridX && curr->y == gridY) {
                // delete coin
                if (prev) prev->next = curr->next;
                else head = curr->next;

                delete curr;
                return true;   // <--- coin was eaten
            }

            prev = curr;
            curr = curr->next;
        }
        return false;  // <--- no coin here
    }

};



// -------------------- Map Class --------------------
class Map {
public:
    vector<string> layout;
    int rows, cols, tileSize;
    CoinList coins;
    vector<pair<int, int>> mysteryPowerUps;   // (x, y)
    unordered_map<int, vector<int>> adjList;

    Map(vector<string> mapLayout, int tSize)
        : layout(mapLayout), tileSize(tSize)
    {
        rows = layout.size();
        cols = layout[0].size();

        // Add coins
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (layout[y][x] == '.')
                    coins.addCoin(x, y);
            }
        }

        // Mystery Power-Ups (add more manually)
        mysteryPowerUps = {
            {16, 1} , {3, 5} ,{4,19}, {15,13}, {10, 15}, {6, 13}
        };

        buildAdjList();
    }

    // ---------------------------------------------------------------
    // Build adjacency list for BFS / ghost pathfinding
    // ---------------------------------------------------------------
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

                    adjList[id].push_back(ny * cols + nx);
                }
            }
        }
    }

    // ---------------------------------------------------------------
    // Draw the map
    // ---------------------------------------------------------------
    void Draw() {
        int pad = 0;

        // Draw walkable background
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                char c = layout[y][x];
                if (c != '#') {
                    DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, BLACK);
                }
            }
        }

        // Walls outline
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (layout[y][x] == '#') {
                    int px = x * tileSize;
                    int py = y * tileSize;

                    if (y == 0 || layout[y - 1][x] != '#')
                        DrawLineEx({ (float)px, (float)py }, { (float)(px + tileSize), (float)py }, 3, DARKBLUE);

                    if (y == rows - 1 || layout[y + 1][x] != '#')
                        DrawLineEx({ (float)px, (float)(py + tileSize) }, { (float)(px + tileSize), (float)(py + tileSize) }, 3, DARKBLUE);

                    if (x == 0 || layout[y][x - 1] != '#')
                        DrawLineEx({ (float)px, (float)py }, { (float)px, (float)(py + tileSize) }, 3, DARKBLUE);

                    if (x == cols - 1 || layout[y][x + 1] != '#')
                        DrawLineEx({ (float)(px + tileSize), (float)py }, { (float)(px + tileSize), (float)(py + tileSize) }, 3, DARKBLUE);
                }
            }
        }

        // Ghost home box
        int gxmin = cols, gxmax = -1, gymin = rows, gymax = -1;
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (layout[y][x] == 'G') {
                    gxmin = min(gxmin, x);
                    gxmax = max(gxmax, x);
                    gymin = min(gymin, y);
                    gymax = max(gymax, y);
                }
            }
        }

        if (gxmax >= gxmin && gymax >= gymin) {
            int left = gxmin * tileSize + pad;
            int top = gymin * tileSize + pad;
            int w = (gxmax - gxmin + 1) * tileSize - pad;
            int h = (gymax - gymin + 1) * tileSize - pad;

            DrawRectangle(left, top, w, h, Color{ 15, 15, 15, 255 });
            DrawRectangleLinesEx({ (float)left, (float)top, (float)w, (float)h }, 2, DARKBLUE);
        }

        // Coins
        coins.drawCoins(tileSize);

        // Large pellets
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (layout[y][x] == 'O') {
                    int cx = x * tileSize + tileSize / 2;
                    int cy = y * tileSize + tileSize / 2;
                    DrawCircle(cx, cy, tileSize * 0.3f, ORANGE);
                }
            }
        }

        // Gate line
        int gateX = 10 * tileSize;
        int gateY = 8 * tileSize;
        DrawLineEx({ (float)gateX, (float)gateY },
            { (float)(gateX + tileSize), (float)gateY },
            3.5f, SKYBLUE);

        // ---------------------------------------------------------------
        // Draw Mystery Power-Ups ("?")
        // ---------------------------------------------------------------
        for (auto& tile : mysteryPowerUps) {
            int cx = tile.first * tileSize + tileSize / 2;
            int cy = tile.second * tileSize + tileSize / 2;

            DrawCircle(cx, cy, tileSize * 0.3f, PURPLE);

            int fontSize = tileSize / 2;
            DrawText("?", cx - fontSize / 4, cy - fontSize / 2, fontSize, WHITE);
        }
    }

    // ---------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------
    bool isWall(int gx, int gy) {
        if (gx < 0 || gx >= cols || gy < 0 || gy >= rows)
            return true;

        char c = layout[gy][gx];
        if (c == '#' || (gy == 9 && gx == 10) || c == 'G')
            return true;

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
    int score = 0;
    int speedTimer;
    int scoreCooldown = 0;   // frames until next score boost allowed
    int speedCooldown = 0;
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
        speed(3.5f), animation_over(false), alive(true),
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
        // Use center-based grid position for accuracy
        int gx = (int)((x + tileSize * 0.5f) / tileSize);
        int gy = (int)((y + tileSize * 0.5f) / tileSize);
        if (gx >= 0 && gx < map.cols && gy >= 0 && gy < map.rows &&
            map.layout[gy][gx] == 'O') {
            map.eatLargePelletAt(gx, gy);
            energizer_timer = 7 * 60;  // 7 seconds
            frightenedTimer = 0;       // Reset for fresh mode
        }
    }

    void speedBoost(int durationFrames) {
        speed += 1.5f;               // temporary boost
        speedTimer = durationFrames;
        speedCooldown = 60;
    }

    void updatePacMan(Map& map, CoinList& coins) {
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
        if (map.coins.eatCoinAt(eatGX, eatGY)) {
            score += 50;
        }

        checkLargePellet(map);

        animation_timer++;

        // ---- handle speed boost timer ----
        if (speedTimer > 0) {
            speedTimer--;
            if (speedTimer == 0) speed = 3.5f; // reset normal speed
        }

        // cooldown countdowns
        if (scoreCooldown > 0) scoreCooldown--;
        if (speedCooldown > 0) speedCooldown--;
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
//Power ups
struct PowerUp {
    int priority;       // bigger = higher priority
    string type;   // "life", "score", "speed"

    bool operator<(const PowerUp& other) const {
        return priority < other.priority; // max-heap
    }
};

// Power-Ups function
static void processMysteryPowerUp(Pacman& pac) {
    priority_queue<PowerUp> pq;
    const int maxLives = 3;

    // 1️⃣ Life has highest priority
    if (pac.lives < maxLives) pq.push({ 3, "life" });

    // 2️⃣ Score boost only if lives full and cooldown is 0
    if (pac.lives == maxLives && pac.scoreCooldown == 0) pq.push({ 2, "score" });

    // 3️⃣ Speed boost if score just applied or allowed
    if (pac.lives == maxLives && pac.scoreCooldown > 0 && pac.speedCooldown == 0)
        pq.push({ 1, "speed" });

    // ---- Process power-ups in priority order ----
    while (!pq.empty()) {
        PowerUp chosen = pq.top();
        pq.pop();

        if (chosen.type == "life" && pac.lives < maxLives) {
            pac.lives++;
        }
        else if (chosen.type == "score") {
            pac.score += 200;
            pac.scoreCooldown = 420; // e.g., 5 seconds before next score boost
        }
        else if (chosen.type == "speed") {
            pac.speedBoost(420); // duration in frames
        }
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
    int gateX, gateY;             // door tile]
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

void backtrackToGate(Ghost& g, Map& map, int tileSize, float speed = 2.5f) {
    using Tile = pair<int, int>;
    uintptr_t key = (uintptr_t)(&g);

    // Static caches keyed by ghost pointer address
    static unordered_map<uintptr_t, vector<Tile>> pathCache;    // tiles from ghost -> ... -> gate
    static unordered_map<uintptr_t, size_t> pathIndex;         // next tile index to move to (start at 1)

    // Gate tile (use eyesTarget if present, otherwise use gateX/gateY)
    int gateX = g.eyesTargetX;
    int gateY = g.eyesTargetY;

    // Current ghost tile
    int gx = (int)((g.position.x + tileSize / 2.0f) / tileSize);
    int gy = (int)((g.position.y + tileSize / 2.0f) / tileSize);

    // If ghost already at gate (tile coords) -> finish immediately
    if (gx == gateX && gy == gateY) {
        // ensure full body and start exit process
        g.frightened_mode = 0;
        g.releaseState = R_EXITING_GATE;
        pathCache.erase(key);
        pathIndex.erase(key);
        return;
    }

    // If there's no cached path or the cached path's start doesn't match current tile, recompute
    bool needCompute = false;
    if (pathCache.find(key) == pathCache.end()) needCompute = true;
    else {
        auto& cached = pathCache[key];
        if (cached.empty()) needCompute = true;
        else {
            // cached[0] should be the ghost tile at time of computation; if ghost moved off that tile,
            // recompute to avoid following stale path.
            if (cached.front().first != gy || cached.front().second != gx) needCompute = true;
        }
    }

    if (needCompute) {
        // BFS from gate -> to ghost tile (so parent pointers allow path reconstruction from ghost -> gate)
        int rows = map.rows;
        int cols = map.cols;
        vector<vector<bool>> visited(rows, vector<bool>(cols, false));
        vector<vector<pair<int, int>>> parent(rows, vector<pair<int, int>>(cols, { -1,-1 }));
        queue<pair<int, int>> q;
        q.push({ gateY, gateX });
        visited[gateY][gateX] = true;

        int dirs[4][2] = { {-1,0},{1,0},{0,-1},{0,1} };
        bool found = false;
        while (!q.empty() && !found) {
            auto cur = q.front(); q.pop();
            for (auto& d : dirs) {
                int ny = cur.first + d[0];
                int nx = cur.second + d[1];
                if (nx >= 0 && nx < cols && ny >= 0 && ny < rows && !visited[ny][nx] && !map.isWall(nx, ny)) {
                    visited[ny][nx] = true;
                    parent[ny][nx] = cur;
                    if (ny == gy && nx == gx) { found = true; break; }
                    q.push({ ny, nx });
                }
            }
        }

        vector<Tile> path; // will hold tiles from ghost -> ... -> gate
        if (found) {
            // reconstruct: start at ghost tile, follow parent -> gate
            pair<int, int> cur = { gy, gx };
            path.push_back(cur);
            while (!(cur.first == gateY && cur.second == gateX)) {
                auto p = parent[cur.first][cur.second];
                if (p.first == -1) break; // safety
                cur = p;
                path.push_back(cur);
            }
            // path[0] = ghost tile, path.back() = gate tile
        }
        else {
            // No path found (shouldn't happen in normal maze). Fallback: single-step attempt to adjacent walkable tile.
            path.push_back({ gy, gx });
            // try to find any adjacent free tile that reduces manhattan to gate
            int bestDist = INT_MAX;
            Tile best = { gy, gx };
            for (auto& d : dirs) {
                int ny = gy + d[0], nx = gx + d[1];
                if (nx >= 0 && nx < cols && ny >= 0 && ny < rows && !map.isWall(nx, ny)) {
                    int manh = abs(nx - gateX) + abs(ny - gateY);
                    if (manh < bestDist) { bestDist = manh; best = { ny,nx }; }
                }
            }
            if (!(best.first == gy && best.second == gx)) path.push_back(best);
        }

        // Cache the path and reset index
        pathCache[key] = move(path);
        pathIndex[key] = 1; // next tile to move to (index 1 means move from tile 0 -> tile 1)
    }

    // Move toward next tile in cached path
    auto& pathRef = pathCache[key];
    size_t& idx = pathIndex[key];

    // If path too short or index out of range -> recompute next frame
    if (pathRef.size() < 2 || idx >= pathRef.size()) {
        // fallback: clear and recompute next frame
        pathCache.erase(key);
        pathIndex.erase(key);
        return;
    }

    // Next tile to approach
    Tile nextT = pathRef[idx];
    float targetX = nextT.second * tileSize + tileSize / 2.0f;
    float targetY = nextT.first * tileSize + tileSize / 2.0f;

    // Smooth movement toward center of next tile
    float cx = g.position.x + tileSize / 2.0f;
    float cy = g.position.y + tileSize / 2.0f;
    float dx = targetX - cx;
    float dy = targetY - cy;
    float len = sqrtf(dx * dx + dy * dy);

    if (len < 0.001f) {
        // snapped onto tile center -> advance index
        // place exactly on tile center to avoid jitter
        g.position.x = nextT.second * tileSize;
        g.position.y = nextT.first * tileSize;
        idx++;
        // If we reached gate tile, finish
        if (idx >= pathRef.size()) {
            pathCache.erase(key);
            pathIndex.erase(key);
            g.frightened_mode = 0;           // show full body
            g.releaseState = R_EXITING_GATE; // resume normal exit
        }
        return;
    }

    // Normalize and move
    float step = speed;
    float vx = dx / len * step;
    float vy = dy / len * step;

    // Before moving, ensure we are drawing full body (per your request to see the body)
    // NOTE: we set frightened_mode to 0 while the body moves back so the normal draw function shows the body.
    g.frightened_mode = 0;

    g.position.x += vx;
    g.position.y += vy;
}

// Flee from Pac-Man: Move in the opposite direction of Pac-Man
void fleeFromPacman(Ghost& g, Pacman& p, Map& map, int tileSize, float speed) {
    // Get Pac-Man's grid position
    int px = (int)((p.x + tileSize / 2) / tileSize);
    int py = (int)((p.y + tileSize / 2) / tileSize);

    // Get ghost's grid position
    int gx = (int)((g.position.x + tileSize / 2) / tileSize);
    int gy = (int)((g.position.y + tileSize / 2) / tileSize);

    // Calculate vector from ghost to Pac-Man
    int dx = px - gx;
    int dy = py - gy;

    // Flee: Move in the opposite direction (away from Pac-Man)
    int fleeX = gx - dx;  // Opposite X
    int fleeY = gy - dy;  // Opposite Y

    // Clamp to map bounds
    if (fleeX < 0) fleeX = 0;
    if (fleeX >= map.cols) fleeX = map.cols - 1;
    if (fleeY < 0) fleeY = 0;
    if (fleeY >= map.rows) fleeY = map.rows - 1;

    // Navigate to the flee target at reduced speed (50% of normal)
    navigateToTile(g, map, tileSize, fleeX, fleeY, speed * 0.5f);

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
        if (frightened_mode == 1) {
            fleeFromPacman(*this, p, m, tileSize, 1.0f);  // Normal speed base, reduced to 50% inside flee
            return;
        }

        // Normal behavior
        if (scatterMode) {
            scatterToCorner(*this, m, tileSize);
        }
        else {
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
        if (frightened_mode == 1) {
            fleeFromPacman(*this, p, m, tileSize, 1.0f);  // Adjust speed as needed
            return;
        }
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
        if (frightened_mode == 1) {
            fleeFromPacman(*this, p, m, tileSize, 1.0f);  // Adjust speed as needed
            return;
        }
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
        if (frightened_mode == 1) {
            fleeFromPacman(*this, p, m, tileSize, 1.0f);  // Adjust speed as needed
            return;
        }

        if (scatterMode) {
            scatterToBottomRight(m, tileSize);
        }
        else {
            chaseTarget(p, red, m, tileSize);
        }
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

    bool flashing = (frightenedTimer >= FRIGHTENED_FLASH_START) && (frightenedTimer % 30 < 15);


    // On first frame of frightened mode, reverse all ghost directions
    if (frightenedTimer == 1) {
        for (Ghost* g : ghosts) {
            if (g->frightened_mode != 2) {
                // Reverse by flipping direction (if used, but BFS overrides)
                g->direction = (g->direction + 2) % 4;
            }
        }
    }
    for (Ghost* g : ghosts) {
        if (g->frightened_mode != 2) {
            g->frightened_mode = 1;
        }
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
        float dx = fabs(pac.x - g->position.x);  //CH1
        float dy = fabs(pac.y - g->position.y);

        if (dx < 20 && dy < 20) // collision
        {
            // Send ghost to cage
            g->frightened_mode = 2; // eyes
            g->position.x = (float)g->cageX * tileSize;
            g->position.y = (float)g->cageY * tileSize;

            // score logic
            pac.score += 200;

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
    if (frightenedTimer >= FRIGHTENED_TOTAL_FRAMES) {
        frightenedTimer = 0;
        for (Ghost* g : ghosts) {
            if (g->frightened_mode != 2) {
                g->frightened_mode = 0;
            }
        }
    }
}

static void checkPacmanGhostCollision(
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
                g->position.x = (float)g->cageX * tileSize;
                g->position.y = (float)g->cageY * tileSize;
                //THESE 2 LINES SEND GHOST BACK TO GATE
                //IN FRIGHTENED MODE AND STOP COLLISION
                // IN FRIGHETENED MODE FROM ACTING LIKE IN SCATTERED M 
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

            

            // Reset Pac-Man position
            pac.x = pac.startXPos;
            pac.y = pac.startYPos;
            pac.direction = Pacman::RIGHT;
            pac.alive = true;

            // Reset all ghosts back to cage
            const int RED_CAGE_COL = 10, RED_CAGE_ROW = 8;
            const int PINK_CAGE_COL = 9, PINK_CAGE_ROW = 9;
            const int ORANGE_CAGE_COL = 10, ORANGE_CAGE_ROW = 9;  // Changed from 10 to 11
            const int BLUE_CAGE_COL = 11, BLUE_CAGE_ROW = 9;

            red.position = { (float)red.cageX * tileSize, (float)red.cageY * tileSize };
            pink.position = { (float)pink.cageX * tileSize, (float)pink.cageY * tileSize };
            orange.position = { (float)orange.cageX * tileSize, (float)orange.cageY * tileSize };
            blue.position = { (float)blue.cageX * tileSize, (float)blue.cageY * tileSize };

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

// ------------MENU--------------

// New standalone function for menu Pacman (add this outside your Pacman class)
void DrawMenuPacman(float x, float y, float radius, int animFrame, Color pacColor = YELLOW) {
    // Center position (simplified from your cx, cy)
    float cx = x + radius;  // Adjust based on how you want centering; this assumes x,y is top-left
    float cy = y + radius;

    // Draw the main body (circle)
    DrawCircle((int)cx, (int)cy, radius, pacColor);

    // Mouth animation: Use a sine wave for opening/closing, but fix direction to RIGHT (facing menu options)
    // Removed direction switch; always face right for simplicity
    float mouthAngle = 30.0f * sin(animFrame * 0.1f);  // Reduced from 40 to 30 for subtler animation; adjust speed (0.1f)
    DrawCircleSector({ cx, cy }, radius, mouthAngle, -mouthAngle, 0, BLACK);  // Mouth sector for RIGHT direction
}

int frameCounter = 0;

void DrawBlinkingTextFrames(const char* text, Vector2 pos, int fontSize, int spacing, Color color, Font titleFont) {
    frameCounter++;
    int blinkSpeed = 30;  // Adjust for faster/slower blink
    if ((frameCounter / blinkSpeed) % 2 == 0) {
        DrawTextEx(titleFont, text, pos, (float)fontSize, (float)spacing, color);
    }
}

void DrawStartScreen(int winW, int winH, Font titleFont, int selectedOption) {
    // Black background
    ClearBackground(BLACK);

    //// Optional: Draw subtle maze elements (e.g., dots)
    //for (int i = 0; i < 20; i++) {
    //    DrawCircle(winW / 2 + (i * 30) - 300, winH / 2, 5, YELLOW);  // Scattered dots
    //}

    // Blinking title (fixed: no duplicate draw)
    const char* title = "PAC-MAZE";
    int titleFontSize = 50;
    int titleSpacing = 4;
    Vector2 titleSize = MeasureTextEx(titleFont, title, (float)titleFontSize, (float)titleSpacing);
    Vector2 titlePos = { (winW - titleSize.x) * 0.5f, 150.0f };
    DrawBlinkingTextFrames(title, titlePos, titleFontSize, titleSpacing, YELLOW, titleFont);

    // Menu options (centered, with highlighting)
    const char* options[] = { "Play", "How to play", "Check Highscore", "Exit" };
    int optionFontSize = 25;
    int optionSpacing = 3;
    Vector2 optionPositions[4];
    float baseY = winH - 300.0f;
    float spacingY = 50.0f;

    for (int i = 0; i < 4; i++) {
        Vector2 size = MeasureTextEx(titleFont, options[i], (float)optionFontSize, (float)optionSpacing);
        optionPositions[i] = Vector2{ (winW - size.x) * 0.5f, baseY + i * spacingY };  // Fixed line
        Color color = (selectedOption == i) ? YELLOW : GRAY;  // Highlight selected
        DrawTextEx(titleFont, options[i], optionPositions[i], (float)optionFontSize, (float)optionSpacing, color);
    }

    // Pacman navigation sprite (moves to selected option)
    float pacmanX = optionPositions[selectedOption].x - 30.0f;  // Position left of text
    float pacmanY = optionPositions[selectedOption].y - 1.0f;  // Center vertically
    // If using texture: DrawTextureEx(pacmanTex, (Vector2){pacmanX, pacmanY}, 0.0f, 0.5f, WHITE);
    // Or simple circle:
    DrawMenuPacman(pacmanX, pacmanY, 10.0f, frameCounter, YELLOW);

    // Footer prompt
    const char* prompt = "Use UP/DOWN to navigate, ENTER to select";
    Vector2 promptSize = MeasureTextEx(titleFont, prompt, 20.0f, 2.0f);
    Vector2 promptPos = { (winW - promptSize.x) * -0.3f, winH - 50.0f };
    DrawTextEx(titleFont, prompt, promptPos, 10.0f, 2.0f, LIGHTGRAY);
}

int loadingFrame = 0;

void DrawLoadingScreen(float& pacX, Font titleFont)
{
    loadingFrame++;

    // Background
    ClearBackground(YELLOW);


    int boxW = 600;
    int boxH = 80;
    int boxX = (GetScreenHeight() - boxW) / 2;
    int boxY = (GetScreenHeight() - boxH) / 2;

    Rectangle boxRect = { (float)boxX, (float)boxY, (float)boxW, (float)boxH };

    // ---------- Rounded Black Box ----------
    DrawRectangleRounded(boxRect, 0.50f, 12, BLACK);

    // ---------- Blue Border ----------
    int radius = 15; // corner radius

    // Sides



    // --- Pellets ---
    int pelletCount = 15;
    int pelletSpacing = 40;
    int pelletY = boxY + boxH / 2;

    for (int i = 0; i < pelletCount; i++)
    {
        int px = boxX + 60 + i * pelletSpacing;

        // Pac-Man "eats" pellets when his X passes them
        if (pacX < px - 10)
            DrawCircle(px, pelletY, 6, WHITE);
    }

    // --- Pac-Man Mouth Animation ---
    float mouthAngle = (loadingFrame % 20 < 10) ? 45.0f : 10.0f;

    // Move Pac-Man
    pacX += 4.0f;
    if (pacX > boxX + boxW - 40)
        pacX = boxX + 20;

    DrawCircleSector(
        { pacX, (float)pelletY },
        24,
        mouthAngle,
        360 - mouthAngle,
        40,
        YELLOW
    );

    Vector2 textPos = { boxX + boxW / 2.0f - 120, boxY + boxH + 40.0f };


    // --- Blinking Loading Text ---
    if ((loadingFrame / 30) % 2 == 0)
        DrawTextEx(titleFont, "LOADING...", textPos, 30.0f, 2.0f, BLACK);
}

bool gameResetFlag = false;  // Global flag to signal a game reset

// Call this function after Pacman loses all lives
void DrawExitScreen(int winW, int winH, Font titleFont, int& gameOverTimer)
{
    static int slideTimer = 0;  // Example static for sliding effect
    static bool effectActive = false;  // Another example static
    static float slideY = -100.0f;  // <-- This is the sliding variable (declared static here)
    if (gameResetFlag) {
        slideTimer = 0;        // Reset sliding timer
        effectActive = false;  // Reset any flags
        slideY = -100.0f;      // <-- ADD THIS: Reset slideY to starting position
        gameResetFlag = false; // Clear the flag after resetting
    }

    float targetY = winH / 2.0f - 50.0f;  // Center vertically (adjust -50 for text height)
    if (slideY < targetY) {
        slideY += 5.0f;  // Speed of slide (adjust for faster/slower)
    }
    if (slideY > targetY) slideY = targetY;
    // Draw "GAME OVER" text in red, centered horizontally
    const char* msg = "GAME OVER";
    int fontSize = 50;
    Vector2 textSize = MeasureTextEx(titleFont, msg, (float)fontSize, 2.0f);
    Vector2 textPos = { (winW - textSize.x) / 2.0f, slideY };  // Centered X, sliding Y
    DrawTextEx(titleFont, msg, textPos, (float)fontSize, 2.0f, RED);

}

// -------------------- DrawHowToScreen (New Function) --------------------
void DrawHowToScreen(int winW, int winH, Font instructionFont) {
    ClearBackground(BLACK);  // Black background

    // Title
    const char* title = "HOW TO PLAY";
    int titleFontSize = 40;
    Vector2 titleSize = MeasureTextEx(instructionFont, title, (float)titleFontSize, 4.0f);
    Vector2 titlePos = { (winW - titleSize.x) / 2.0f, 100.0f };
    DrawTextEx(instructionFont, title, titlePos, (float)titleFontSize, 4.0f, YELLOW);

    // Instructions (bullet points)
    int instrFontSize = 10;
    float instrSpacing = 2.0f;
    float startY = 200.0f;
    float lineHeight = 30.0f;

    const char* instructions[] = {
        "- Use ARROW KEYS to move Pacman.",
        "- Eat all small pellets (dots) to win the level.",
        "- Eat large pellets (big dots) to" ,
        " make ghosts edible (blue).",
        "- Avoid ghosts when they are not blue,",
        " or lose a life.",
        "- You have 3 lives. Lose all to game over.",
        "- Press ENTER or ESCAPE to return to menu."
    };

    for (int i = 0; i < 6; i++) {
        Vector2 instrSize = MeasureTextEx(instructionFont, instructions[i], (float)instrFontSize, instrSpacing);
        Vector2 instrPos = { (winW - instrSize.x) / 2.0f, startY + i * lineHeight };
        DrawTextEx(instructionFont, instructions[i], instrPos, (float)instrFontSize, instrSpacing, WHITE);
    }

    // Footer prompt
    const char* prompt = "Press ENTER or ESCAPE to go back";
    Vector2 promptSize = MeasureTextEx(instructionFont, prompt, 15.0f, 2.0f);
    Vector2 promptPos = { (winW - promptSize.x) / 2.0f, winH - 50.0f };
    DrawTextEx(instructionFont, prompt, promptPos, 15.0f, 2.0f, LIGHTGRAY);
}

// -------------------- DrawEnterNameScreen (New Function) --------------------
void DrawEnterNameScreen(int winW, int winH, Font titleFont, string& playerName) {
    ClearBackground(BLACK);  // Black background

    // Title
    const char* title = "ENTER YOUR NAME";
    int titleFontSize = 30;
    Vector2 titleSize = MeasureTextEx(titleFont, title, (float)titleFontSize, 4.0f);
    Vector2 titlePos = { (winW - titleSize.x) / 2.0f, 150.0f };
    DrawTextEx(titleFont, title, titlePos, (float)titleFontSize, 4.0f, YELLOW);

    // Input box (simple rectangle)
    float boxX = winW / 2.0f - 150.0f;
    float boxY = 250.0f;
    float boxW = 300.0f;
    float boxH = 50.0f;
    DrawRectangle(boxX, boxY, boxW, boxH, DARKGRAY);
    DrawRectangleLinesEx({ boxX, boxY, boxW, boxH }, 2, WHITE);

    // Display entered name
    int nameFontSize = 25;
    Vector2 nameSize = MeasureTextEx(titleFont, playerName.c_str(), (float)nameFontSize, 2.0f);
    Vector2 namePos = { boxX + 10.0f, boxY + (boxH - nameSize.y) / 2.0f };
    DrawTextEx(titleFont, playerName.c_str(), namePos, (float)nameFontSize, 2.0f, WHITE);

    // Cursor (blinking)
    static int cursorTimer = 0;
    cursorTimer++;
    if ((cursorTimer / 30) % 2 == 0) {  // Blink every 30 frames
        float cursorX = namePos.x + nameSize.x + 5.0f;
        float cursorY = namePos.y;
        DrawLineEx({ cursorX, cursorY }, { cursorX, cursorY + nameSize.y }, 2.0f, WHITE);
    }

    // Instructions
    const char* instr1 = "Type your name (max 10 chars)";
    Vector2 instr1Size = MeasureTextEx(titleFont, instr1, 15.0f, 2.0f);
    Vector2 instr1Pos = { (winW - instr1Size.x) / 2.0f, boxY + boxH + 80.0f };
    DrawTextEx(titleFont, instr1, instr1Pos, 15.0f, 2.0f, LIGHTGRAY);


    const char* instr2 = "and press ENTER";
    Vector2 instr2Size = MeasureTextEx(titleFont, instr2, 15.0f, 2.0f);
    Vector2 instr2Pos = { (winW - instr2Size.x) / 2.0f, boxY + boxH + 110.0f };
    DrawTextEx(titleFont, instr2, instr2Pos, 15.0f, 2.0f, LIGHTGRAY);
}

// -------------------- resetGame (New Function) --------------------
void resetGame(Map& maze, Pacman& pac, RedGhost& red, PinkGhost& pink, OrangeGhost& orange, BlueGhost& blue,
vector<string>& originalLayout, int tileSize, int& frightenedTimer,
int& globalFrames, int& waveTimer, bool& scatterMode,
ReleaseInfo& redRelease, ReleaseInfo& pinkRelease, ReleaseInfo& orangeRelease, ReleaseInfo& blueRelease,
int& pacEnergizerTimer)  // <-- Add this parameter
    {
    // Reset maze
    maze.layout = originalLayout;  // Reload original layout
    maze.coins = CoinList();       // Clear and rebuild coins
    for (int y = 0; y < maze.rows; y++) {
        for (int x = 0; x < maze.cols; x++) {
            if (maze.layout[y][x] == '.') maze.coins.addCoin(x, y);
        }
    }

    // Reset Pacman
    pac.x = pac.startXPos;
    pac.y = pac.startYPos;
    pac.direction = Pacman::RIGHT;
    pac.desiredDirection = Pacman::RIGHT;
    pac.alive = true;
    pac.dying = false;
    pac.death_timer = 0;
    pac.lives = 3;
    pac.energizer_timer = 0;
    pac.score = 0;


    // Reset ghosts
    red.position = { 10.f * tileSize, 8.f * tileSize };
    pink.position = { 9.f * tileSize, 9.f * tileSize };
    orange.position = { 10.f * tileSize, 9.f * tileSize };
    blue.position = { 11.f * tileSize, 9.f * tileSize };
    red.frightened_mode = pink.frightened_mode = orange.frightened_mode = blue.frightened_mode = 0;

    // Reset release states
    redRelease.state = R_ACTIVE;
    pinkRelease.state = orangeRelease.state = blueRelease.state = R_IN_CAGE;
    pinkRelease.timer = orangeRelease.timer = blueRelease.timer = 0;

    // Reset timers and modes
    frightenedTimer = 0;
    pacEnergizerTimer = 0;
    globalFrames = 0;
    waveTimer = 0;
    scatterMode = true;
    gameResetFlag = true;


    // Reset static variables for game over screen
    // Note: These are static in DrawExitScreen and game over section, so we can't directly reset them here.
    // Instead, we'll handle it in the game over section by setting flags.
}




// -------------------- Main --------------------
int main() {
    vector<string> mazeLayout = {
        " ################### ",
        " #.O......#..... ..# ",
        " #.##.###.#.###.##.# ",
        " #.................# ",
        " #.##.#.#####.#.##.# ",
        " #. ..#...#...#....# ",
        " ####.### # ###.#### ",
        "    #.#       #.#    ",
        "#####.# GGGGG #.#####",
        "     .  #GGG#  .     ",
        "#####.# GGGGG #.#####",
        "    #.#....O..#.#    ",
        " ####.#.#####.#.#### ",
        " #.. .....#.... ...# ",
        " #.##.###.#.###.##.# ",
        " #..#.O... .....#..# ",
        " ##.#.#.#####.P.#.## ",
        " #....#...#...#....# ",
        " #.######.#.######.# ",
        " #.. ............O.# ",
        " ################### "
    };

    int tileSize = 30;
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
    InitAudioDevice();  // Add this line to initialize audio

    SetTargetFPS(60);

    Sound gameOverSound = LoadSound("gameover2.mp3");  // Replace with your sound file path


    // Game state
    MenuOption selectedOption = MENU_PLAY;
    GameState currentState = STATE_LOADING;




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
    int gameOverTimer = 0;  // New: Timer for game over screen duration
    bool nameEntered = false;  // Track if player has entered name (only once at start)
    int pacEnergizerTimer = 0;  // <-- NEW: Declare this as an int (initialize to 0 or your default)



    Font titleFont = LoadFont("pacman_font.ttf");  // Replace with your font file
    Font instructionFont = LoadFont("instruction.ttf");


    // -------------------- GAME LOOP --------------------
    while (!WindowShouldClose()) {

        BeginDrawing();

        static float loadPacX = 0.0f;

        // ---------------- LOADING SCREEN ----------------
        if (currentState == STATE_LOADING)
        {
            ClearBackground(BLACK);
            DrawLoadingScreen(loadPacX, titleFont);


            loadingFrame++;
            if (loadingFrame > 240) {
                if (!nameEntered) {
                    currentState = STATE_ENTER_NAME;  // First time: go to enter name
                    playerName = "";  // Reset name
                }
                else {
                    currentState = STATE_MENU;  // Subsequent times: skip to menu
                    selectedOption = MENU_PLAY;
                }
            }


            EndDrawing();
            continue;
        }

        // ---------------- EXIT / GAME OVER SCREEN ----------------
        if (pac.lives <= 0)
        {
            ClearBackground(BLACK);
            DrawExitScreen(winW, winH, titleFont, gameOverTimer);
            static bool soundPlayed = false;
            if (!soundPlayed) {
                PlaySound(gameOverSound);
                soundPlayed = true;
            }

            gameOverTimer++;

            if (gameOverTimer >= 150) {
                resetGame(maze, pac, red, pink, orange, blue, mazeLayout, tileSize,
                    frightenedTimer, globalFrames, waveTimer, scatterMode,
                    redRelease, pinkRelease, orangeRelease, blueRelease,
                    pacEnergizerTimer);  // <-- Pass the new parameter


                // Reset static flags for next game over
                soundPlayed = false;
                // slideY is reset in DrawExitScreen via static, but we can force it by calling a reset if needed
                // For now, since DrawExitScreen initializes slideY = -100.0f each time, it should be fine


                gameOverTimer = 0;
                currentState = STATE_LOADING;
                loadingFrame = 0;
                loadPacX = 0.0f;
            }
            EndDrawing();
            continue;
        }

        if (currentState == STATE_ENTER_NAME) {
            DrawEnterNameScreen(winW, winH, titleFont, playerName);
            // Handle input
            int key = GetKeyPressed();
            if (key >= 32 && key <= 126 && playerName.length() < 10) {  // Printable characters, max 10
                playerName += (char)key;
            }
            if (IsKeyPressed(KEY_BACKSPACE) && !playerName.empty()) {
                playerName.pop_back();
            }
            if (IsKeyPressed(KEY_ENTER) && !playerName.empty()) {
                nameEntered = true;  // Mark as entered

                currentState = STATE_MENU;  // Proceed to menu if name is entered
                selectedOption = MENU_PLAY;
            }
            EndDrawing();
            continue;
        }



        // -------------------- START MENU --------------------
        if (currentState == STATE_MENU) {
            // Handle navigation
            if (IsKeyPressed(KEY_DOWN)) {
                selectedOption = static_cast<MenuOption>((static_cast<int>(selectedOption) + 1) % 4);
            }

            if (IsKeyPressed(KEY_UP)) {
                selectedOption = static_cast<MenuOption>((static_cast<int>(selectedOption) - 1 + 4) % 4);
            }
            if (IsKeyPressed(KEY_ENTER)) {
                switch (selectedOption) {
                case MENU_PLAY:
                    currentState = STATE_PLAYING;
                    globalFrames = 0;
                    waveTimer = 0;
                    break;
                case MENU_HOW_TO:
                    currentState = STATE_HOW_TO;  // Implement this state/screen
                    break;
                case MENU_HIGHSCORE:
                    currentState = STATE_HIGHSCORE;  // Implement this state/screen
                    break;
                case MENU_EXIT:
                    CloseWindow();  // Exit the game entirely
                    return 0;
                    break;
                }
            }

            DrawStartScreen(winW, winH, titleFont, selectedOption);
            EndDrawing();
            continue;
        }

        // -------------------- HOW TO PLAY --------------------

        if (currentState == STATE_HOW_TO) {
            DrawHowToScreen(winW, winH, titleFont);
            // Handle input to return to menu
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
                currentState = STATE_MENU;
                selectedOption = MENU_PLAY;  // Reset to "Play" for convenience
            }
            EndDrawing();
            continue;  // Skip rest of loop
        }


        // -------------------- GAMEPLAY MODE --------------------
        ClearBackground(BLACK);
        globalFrames++;

        int pacGridX = (int)((pac.x + pac.tileSize / 2) / pac.tileSize);
        int pacGridY = (int)((pac.y + pac.tileSize / 2) / pac.tileSize);

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
                red.position = { (float)red.cageX * tileSize, (float)red.cageY * tileSize };
                pink.position = { (float)pink.cageX * tileSize, (float)pink.cageY * tileSize };
                orange.position = { (float)orange.cageX * tileSize, (float)orange.cageY * tileSize };  // Now correctly x=11
                blue.position = { (float)blue.cageX * tileSize, (float)blue.cageY * tileSize };        // Now correctly x=10


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
            CoinList coins;
            pac.updatePacMan(maze, coins);

            // 2️⃣ Check for large pellet

            // -------------------- Update ghosts (call in your main loop) --------------------

// Red
            if (red.frightened_mode == 2) {
                // eyes-eaten state → backtrack to gate using BFS
                backtrackToGate(red, maze, tileSize, 1.0f);
            }
            else {
                // normal behaviour (frightened_mode==1 handled inside update if you implemented it)
                red.update(pac, maze, tileSize, scatterMode);
            }

            // Pink
            if (pink.frightened_mode == 2) {
                backtrackToGate(pink, maze, tileSize, 1.0);
            }
            else {
                pink.update(pac, maze, tileSize, scatterMode);
            }

            // Orange
            if (orange.frightened_mode == 2) {
                backtrackToGate(orange, maze, tileSize, 1.0f);
            }
            else {
                orange.update(pac, maze, tileSize, scatterMode);
            }

            // Blue (note: Blue's update needs Red reference)
            if (blue.frightened_mode == 2) {
                backtrackToGate(blue, maze, tileSize, 1.0f);
            }
            else {
                blue.update(pac, red, maze, tileSize, scatterMode);
            }



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
        bool flashing = (frightenedTimer >= FRIGHTENED_FLASH_START) && (frightenedTimer % 30 < 15);

        red.draw(flashing, tileSize);
        pink.draw(flashing, tileSize);
        orange.draw(flashing, tileSize);
        blue.draw(flashing, tileSize);

        DrawLives(pac.lives, tileSize, winW);

        DrawLives(pac.lives, tileSize, winW);

        string scoreText = "SCORE: " + to_string(pac.score);
        DrawTextEx(titleFont, scoreText.c_str(), { 10, 10 }, 12, 2, WHITE);

        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();

        string speedText = "Speed: " + to_string((int)(pac.speed * 10) / 10.0f);
        DrawText(speedText.c_str(), 10, screenHeight - 30, 20, GREEN);

        EndDrawing();
    }

    UnloadTexture(ghostTexture);
    UnloadSound(gameOverSound);

    CloseWindow();
    return 0;
}
