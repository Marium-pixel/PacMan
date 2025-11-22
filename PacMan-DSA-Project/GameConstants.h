#pragma once
#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H

#include <string>
#include <vector>
using namespace std;

namespace GameConstants {
    // Tile and animation constants
    constexpr unsigned char CELL_SIZE = 16;
    constexpr unsigned char MAP_HEIGHT = 21;
    constexpr unsigned char MAP_WIDTH = 21;
    constexpr unsigned char GHOST_ANIMATION_FRAMES = 2;
    constexpr unsigned char GHOST_ANIMATION_SPEED = 8;

    // Frightened mode constants
    const int FRIGHTENED_TOTAL_FRAMES = 60 * 7;
    const int FRIGHTENED_FLASH_START = FRIGHTENED_TOTAL_FRAMES - (60 * 2);
    const int FRIGHTENED_FLASH_INTERVAL = 15;

    // Other constants (e.g., collision, tile size)
    const float PACMAN_GHOST_COLLISION_DIST = 20.0f;
    const int TILE_SIZE = 45;

    // Enums
    typedef enum { MENU_PLAY, MENU_HOW_TO, MENU_HIGHSCORE, MENU_EXIT } MenuOption;
    typedef enum { STATE_MENU, STATE_LOADING, STATE_ENTER_NAME, STATE_PLAYING, STATE_HIGHSCORE, STATE_HOW_TO, STATE_EXIT } GameState;

    enum GhostState { NORMAL, FRIGHTENED, EYES };
    enum ReleaseState { R_IN_CAGE = 0, R_EXITING_GATE = 1, R_ACTIVE = 2 };
} // namespace GameConstants

// Structs (if needed globally)
struct ReleaseInfo {
    GameConstants::ReleaseState state;
    int timer;
    bool justEnteredScatter;
    ReleaseInfo() : state(GameConstants::R_IN_CAGE), timer(0), justEnteredScatter(false) {}
};


#endif // GAME_CONSTANTS_H
