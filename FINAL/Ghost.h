#pragma once
#ifndef GHOST_H
#define GHOST_H

#include "raylib.h"
#include "GameConstants.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <climits>

using namespace GameConstants;   

class Map;
class Pacman;
class RedGhost; // Forward declaration for BlueGhost


// -------------------- Ghost Base Class --------------------
class Ghost {
public:
    Vector2 position;
    int id;
    int direction;
    int frightened_mode;          // 0=normal,1=frightened,2=eyes
    int animation_timer;
    Texture2D texture;

    bool isHard;
    // speeds (tiles-per-frame or whatever units you're using)
    static constexpr float NORMAL_SPEED = 1.5f;
    static constexpr float HARD_SPEED = 2.5f;

    ReleaseState releaseState = R_IN_CAGE;
    float speed;


    int cageX, cageY;             // cage center
    int gateX, gateY;             // door tile
    int eyesTargetX, eyesTargetY; // tile to move toward when eaten

    Ghost(int gx, int gy, int i_id, Texture2D tex, int tileSize);

    void moveToGate(Map& map, int tileSize);
    void draw(bool i_flash, int tileSize);
    void updateReleaseState(Map& map, float tileSize);
    // toggles hard mode for this ghost at runtime
    void setHardMode(bool hard);

};

// -------------------- Specific Ghosts --------------------
class RedGhost : public Ghost {
public:
    RedGhost(int gx, int gy, int id, Texture2D tex, int tile);
    void update(Pacman& p, Map& m, int tileSize, bool scatterMode);
    void draw(bool debug, int tileSize);
};

class PinkGhost : public Ghost {
public:
    PinkGhost(int gx, int gy, int id, Texture2D tex, int tile);
    void scatterToTopLeft(Map& m, int tileSize);
    void chaseTarget(Pacman& p, Map& m, int tileSize);
    void update(Pacman& p, Map& m, int tileSize, bool scatterMode);
    void draw(bool debug, int tileSize);
};

class OrangeGhost : public Ghost {
public:
    OrangeGhost(int gx, int gy, int id, Texture2D tex, int tile);
    void scatterToBottomLeft(Map& m, int tileSize);
    void chaseTarget(Pacman& p, Map& m, int tileSize);
    void update(Pacman& p, Map& m, int tileSize, bool scatterMode);
    void draw(bool debug, int tileSize);
};

class BlueGhost : public Ghost {
public:
    BlueGhost(int gx, int gy, int id, Texture2D tex, int tile);
    void scatterToBottomRight(Map& m, int tileSize);
    void chaseTarget(Pacman& p, RedGhost& red, Map& m, int tileSize);
    void update(Pacman& p, RedGhost& red, Map& m, int tileSize, bool scatterMode);
    void draw(bool debug, int tileSize);
};

// -------------------- Ghost Functions --------------------
void navigateToTile(Ghost& g, Map& map, int tileSize, int tx, int ty, float speed);

void backtrackToGate(Ghost& g, Map& map, int tileSize, float speed = 2.0f);

void fleeFromPacman(Ghost& g, Pacman& p, Map& map, int tileSize, float speed);

void chasePacmanToTarget(Ghost& g, Pacman& p, Map& map, int tileSize);

void scatterToCorner(Ghost& g, Map& map, int tileSize);

void navigateGhostToTile(Ghost& g, Map& map, int tileSize, int targetCol, int targetRow, float speed);

void releaseGhost(Ghost& g, Ghost& red, Map& map, int tileSize, int framesSinceStart, ReleaseInfo& info);

void frightened(Pacman& pac, RedGhost& red, PinkGhost& pink, OrangeGhost& orange, BlueGhost& blue,
    int& frightenedTimer, int tileSize,
    ReleaseInfo& redInfo, ReleaseInfo& pinkInfo, ReleaseInfo& orangeInfo, ReleaseInfo& blueInfo,
    Map& map, int& framesSinceStart);

void checkPacmanGhostCollision(Pacman& pac,
    RedGhost& red, PinkGhost& pink, OrangeGhost& orange, BlueGhost& blue,
    int tileSize,
    ReleaseInfo& redInfo, ReleaseInfo& blueInfo, ReleaseInfo& pinkInfo, ReleaseInfo& orangeInfo,
    Map& map, int& framesSinceStart);


#endif // GHOST_H
