#include "Pacman.h"
#include "Map.h" // needed for Map methods
#include <cmath>
#include <algorithm>

using namespace std;

// -------------------- Pacman Methods --------------------
Pacman::Pacman(int startX, int startY, int tSize)
    : x((float)startX), y((float)startY), tileSize(tSize),
    speed(3.5f), animation_over(false), alive(true),
    direction(RIGHT), desiredDirection(RIGHT), animation_timer(0),
    energizer_timer(0), lives(3), dying(false), death_timer(0),
    score(300), speedTimer(0), scoreCooldown(0), speedCooldown(0)
{
    radius = tileSize * 0.2f;
    startXPos = (float)x;
    startYPos = (float)y;
}

void Pacman::resetPosition() {
    alive = true;
    x = startXPos;
    y = startYPos;
    direction = RIGHT;
    desiredDirection = RIGHT;
}

// Collision helpers
bool Pacman::circleIntersectsRect(float cx, float cy, float r,
    float rx, float ry, float rw, float rh)
{
    float nearestX = max(rx, min(cx, rx + rw));
    float nearestY = max(ry, min(cy, ry + rh));
    float dx = cx - nearestX;
    float dy = cy - nearestY;
    return (dx * dx + dy * dy) < (r * r);
}

bool Pacman::collidesAtCenter(Map& map, float centerX, float centerY) {
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

void Pacman::checkLargePellet(Map& map) {
    int gx = (int)((x + tileSize * 0.5f) / tileSize);
    int gy = (int)((y + tileSize * 0.5f) / tileSize);
    if (gx >= 0 && gx < map.cols && gy >= 0 && gy < map.rows &&
        map.layout[gy][gx] == 'O') {
        map.eatLargePelletAt(gx, gy);
        energizer_timer = 7 * 60;  // 7 seconds
    }
}

void Pacman::speedBoost(int durationFrames) {
    speed += 1.5f;
    speedTimer = durationFrames;
    speedCooldown = 60;
}

void Pacman::updatePacMan(Map& map, CoinList& coins) {
    if (dying) {
        death_timer++;
        if (death_timer >= DEATH_FRAMES) {
            dying = false;
            death_timer = 0;
            x = startXPos;
            y = startYPos;
            direction = RIGHT;
            desiredDirection = RIGHT;
            alive = true;
        }
        return;
    }

    // Movement and input
    int gridX = (int)((x + tileSize / 2) / tileSize);
    int gridY = (int)((y + tileSize / 2) / tileSize);
    float centerX = gridX * tileSize + tileSize / 2.0f;
    float centerY = gridY * tileSize + tileSize / 2.0f;

    if (fabs(x - centerX) < 0.4f) x = centerX;
    if (fabs(y - centerY) < 0.4f) y = centerY;

    if (IsKeyDown(KEY_RIGHT)) desiredDirection = RIGHT;
    if (IsKeyDown(KEY_LEFT)) desiredDirection = LEFT;
    if (IsKeyDown(KEY_UP)) desiredDirection = UP;
    if (IsKeyDown(KEY_DOWN)) desiredDirection = DOWN;

    auto canMove = [&](Direction dir) -> bool {
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

    if (map.coins.eatCoinAt(eatGX, eatGY)) score += 50;

    checkLargePellet(map);

    animation_timer++;
    if (speedTimer > 0) { speedTimer--; if (speedTimer == 0) speed = 3.5f; }
    if (scoreCooldown > 0) scoreCooldown--;
    if (speedCooldown > 0) speedCooldown--;
}

void Pacman::draw(bool victory) {
    float cx = x + tileSize / 2;
    float cy = y + tileSize / 2;

    if (dying) {
        float t = (float)death_timer / DEATH_FRAMES;
        float deathRadius = radius * (1.0f - 0.5f * t);
        float mouthAngle = 180.0f * t;
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

// -------------------- PowerUp --------------------
bool PowerUp::operator<(const PowerUp& other) const {
    return priority < other.priority;
}

void processMysteryPowerUp(Pacman& pac) {
    priority_queue<PowerUp> pq;
    const int maxLives = 3;

    if (pac.lives < maxLives) pq.push({ 3, "life" });
    if (pac.lives == maxLives && pac.scoreCooldown == 0) pq.push({ 2, "score" });
    if (pac.lives == maxLives && pac.scoreCooldown > 0 && pac.speedCooldown == 0) pq.push({ 1, "speed" });

    while (!pq.empty()) {
        PowerUp chosen = pq.top();
        pq.pop();
        if (chosen.type == "life" && pac.lives < maxLives) pac.lives++;
        else if (chosen.type == "score") { pac.score += 200; pac.scoreCooldown = 420; }
        else if (chosen.type == "speed") pac.speedBoost(420);
    }
}
