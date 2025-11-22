#pragma once
#ifndef PACMAN_H
#define PACMAN_H

#include "raylib.h"
#include "GameConstants.h"
#include <queue>
#include <string>
using namespace std;

class Map; // forward declaration
class CoinList; // forward declaration

class Pacman {
public:
    // Position & movement
    float x, y;
    int tileSize;
    float speed;
    float radius;

    // Game state
    int lives;
    int score;
    int speedTimer;
    int scoreCooldown;
    int speedCooldown;
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
    Direction desiredDirection;

    // Constructor
    Pacman(int startX, int startY, int tSize);

    // Methods
    void resetPosition();
    void updatePacMan(Map& map, CoinList& coins);
    void draw(bool victory = false);
    void checkLargePellet(Map& map);
    void speedBoost(int durationFrames);

    // Collision helper
    bool collidesAtCenter(Map& map, float centerX, float centerY);
    static bool circleIntersectsRect(float cx, float cy, float r,
        float rx, float ry, float rw, float rh);
};

// PowerUp struct
struct PowerUp {
    int priority;       // bigger = higher priority
    std::string type;   // "life", "score", "speed"
    bool operator<(const PowerUp& other) const;
};

// Power-up processing function
void processMysteryPowerUp(Pacman& pac);

#endif // PACMAN_H
