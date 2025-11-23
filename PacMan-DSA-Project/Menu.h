#pragma once
#ifndef MENU_H
#define MENU_H

#include "raylib.h"
#include "GameConstants.h"
#include <string>
#include <vector>
#include "Map.h"
#include "Pacman.h"
#include "Highscore.h"
//#include "Highscore.cpp"
#include "Ghost.h"  // Includes RedGhost, PinkGhost, OrangeGhost, BlueGhost

using namespace std;

extern string playerName;
extern int loadingFrame;

class GameStateManager {
public:
    GameConstants::GameState currentState;
    GameConstants::MenuOption selectedOption;

    void setState(GameConstants::GameState newState) { currentState = newState; }
};


// Menu-related drawing functions
void DrawLives(int lives, int tileSize, int screenWidth);
void DrawMenuPacman(float x, float y, float radius, int animFrame, Color pacColor = YELLOW);
void DrawBlinkingTextFrames(const char* text, Vector2 pos, int fontSize, int spacing, Color color, Font titleFont);
void DrawLevelSelectScreen(int winW, int winH, Font font, int selected);

void DrawStartScreen(int winW, int winH, Font titleFont, int selectedOption);
void DrawHowToScreen(int winW, int winH, Font instructionFont);
void DrawEnterNameScreen(int winW, int winH, Font titleFont, string& playerName);
void DrawLoadingScreen(float& pacX, Font titleFont);
void DrawExitScreen(int winW, int winH, Font titleFont, int& gameOverTimer);

// Reset game function
void resetGame(Map& maze, Pacman& pac, RedGhost& red, PinkGhost& pink, OrangeGhost& orange, BlueGhost& blue,
    vector<string>& originalLayout, int tileSize, int& frightenedTimer,
    int& globalFrames, int& waveTimer, bool& scatterMode,
    ReleaseInfo& redRelease, ReleaseInfo& pinkRelease, ReleaseInfo& orangeRelease, ReleaseInfo& blueRelease,
    int& pacEnergizerTimer);

// Global flag to signal game reset
extern bool gameResetFlag;

#endif // MENU_H#pragma once
