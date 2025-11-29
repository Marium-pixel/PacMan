#include "Menu.h"
#include <cmath>

bool gameResetFlag = false;  // define the global flag

int loadingFrame = 0;
string playerName = "";

// -------------------- Draw Lives --------------------
void DrawLives(int lives, int tileSize, int screenWidth) {
    float startX = screenWidth - (lives * 35) - 20; // 20px margin
    float y = 25;
    float r = tileSize * 0.25f;

    for (int i = 0; i < lives; i++) {
        float lx = startX + i * 35;
        float ly = y;
        float open = 40;

        DrawCircle(lx, ly, r, YELLOW);
        DrawCircleSector({ lx, ly }, r, open, -open, 0, BLACK);
    }
}

// -------------------- Draw Menu Pacman --------------------
void DrawMenuPacman(float x, float y, float radius, int animFrame, Color pacColor) {
    float cx = x + radius;
    float cy = y + radius;
    DrawCircle((int)cx, (int)cy, radius, pacColor);
    float mouthAngle = 30.0f * sin(animFrame * 0.1f);
    DrawCircleSector({ cx, cy }, radius, mouthAngle, -mouthAngle, 0, BLACK);
}

// -------------------- Blinking Text --------------------
int frameCounter = 0;
void DrawBlinkingTextFrames(const char* text, Vector2 pos, int fontSize, int spacing, Color color, Font titleFont) {
    frameCounter++;
    int blinkSpeed = 30;
    if ((frameCounter / blinkSpeed) % 2 == 0) {
        DrawTextEx(titleFont, text, pos, (float)fontSize, (float)spacing, color);
    }
}



// -------------------- Start Screen --------------------
void DrawStartScreen(int winW, int winH, Font titleFont, int selectedOption) {
    ClearBackground(BLACK);

    const char* title = "PAC-MAZE";
    int titleFontSize = 50;
    int titleSpacing = 4;
    Vector2 titleSize = MeasureTextEx(titleFont, title, (float)titleFontSize, (float)titleSpacing);
    Vector2 titlePos = { (winW - titleSize.x) * 0.5f, 150.0f };
    DrawBlinkingTextFrames(title, titlePos, titleFontSize, titleSpacing, YELLOW, titleFont);

    const char* options[] = { "Play", "How to play", "Check Highscore", "Exit" };
    int optionFontSize = 25;
    int optionSpacing = 3;
    Vector2 optionPositions[4];
    float baseY = winH - 300.0f;
    float spacingY = 50.0f;

    for (int i = 0; i < 4; i++) {
        Vector2 size = MeasureTextEx(titleFont, options[i], (float)optionFontSize, (float)optionSpacing);
        optionPositions[i] = Vector2{ (winW - size.x) * 0.5f, baseY + i * spacingY };
        Color color = (selectedOption == i) ? YELLOW : GRAY;
        DrawTextEx(titleFont, options[i], optionPositions[i], (float)optionFontSize, (float)optionSpacing, color);
    }

    float pacmanX = optionPositions[selectedOption].x - 30.0f;
    float pacmanY = optionPositions[selectedOption].y - 1.0f;
    DrawMenuPacman(pacmanX, pacmanY, 10.0f, frameCounter, YELLOW);

    const char* prompt = "Use UP/DOWN to navigate, ENTER to select";
    Vector2 promptSize = MeasureTextEx(titleFont, prompt, 20.0f, 2.0f);
    Vector2 promptPos = { (winW - promptSize.x) * -0.3f, winH - 50.0f };
    DrawTextEx(titleFont, prompt, promptPos, 10.0f, 2.0f, LIGHTGRAY);
}

// -------------------- How To Play --------------------
void DrawHowToScreen(int winW, int winH, Font instructionFont) {
    ClearBackground(BLACK);
    const char* title = "HOW TO PLAY";
    int titleFontSize = 40;
    Vector2 titleSize = MeasureTextEx(instructionFont, title, (float)titleFontSize, 4.0f);
    Vector2 titlePos = { (winW - titleSize.x) / 2.0f, 100.0f };
    DrawTextEx(instructionFont, title, titlePos, (float)titleFontSize, 4.0f, YELLOW);

    int instrFontSize = 10;
    float instrSpacing = 2.0f;
    float startY = 200.0f;
    float lineHeight = 30.0f;

    const char* instructions[] = {
        "- Use ARROW KEYS to move Pacman.",
        "- Eat all small pellets (dots) to win the level.",
        "- Eat large pellets (big dots) to",
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

    const char* prompt = "Press ENTER or ESCAPE to go back";
    Vector2 promptSize = MeasureTextEx(instructionFont, prompt, 15.0f, 2.0f);
    Vector2 promptPos = { (winW - promptSize.x) / 2.0f, winH - 50.0f };
    DrawTextEx(instructionFont, prompt, promptPos, 15.0f, 2.0f, LIGHTGRAY);
}

// -------------------- Enter Name --------------------
void DrawEnterNameScreen(int winW, int winH, Font titleFont, string& playerName) {
    ClearBackground(BLACK);

    const char* title = "ENTER YOUR NAME";
    int titleFontSize = 30;
    Vector2 titleSize = MeasureTextEx(titleFont, title, (float)titleFontSize, 4.0f);
    Vector2 titlePos = { (winW - titleSize.x) / 2.0f, 150.0f };
    DrawTextEx(titleFont, title, titlePos, (float)titleFontSize, 4.0f, YELLOW);

    float boxX = winW / 2.0f - 150.0f;
    float boxY = 250.0f;
    float boxW = 300.0f;
    float boxH = 50.0f;
    DrawRectangle(boxX, boxY, boxW, boxH, DARKGRAY);
    DrawRectangleLinesEx({ boxX, boxY, boxW, boxH }, 2, WHITE);

    int nameFontSize = 25;
    Vector2 nameSize = MeasureTextEx(titleFont, playerName.c_str(), (float)nameFontSize, 2.0f);
    Vector2 namePos = { boxX + 10.0f, boxY + (boxH - nameSize.y) / 2.0f };
    DrawTextEx(titleFont, playerName.c_str(), namePos, (float)nameFontSize, 2.0f, WHITE);

    static int cursorTimer = 0;
    cursorTimer++;
    if ((cursorTimer / 30) % 2 == 0) {
        float cursorX = namePos.x + nameSize.x + 5.0f;
        float cursorY = namePos.y;
        DrawLineEx({ cursorX, cursorY }, { cursorX, cursorY + nameSize.y }, 2.0f, WHITE);
    }

    const char* instr1 = "Type your name (max 10 chars)";
    Vector2 instr1Size = MeasureTextEx(titleFont, instr1, 15.0f, 2.0f);
    Vector2 instr1Pos = { (winW - instr1Size.x) / 2.0f, boxY + boxH + 80.0f };
    DrawTextEx(titleFont, instr1, instr1Pos, 15.0f, 2.0f, LIGHTGRAY);

    const char* instr2 = "and press ENTER";
    Vector2 instr2Size = MeasureTextEx(titleFont, instr2, 15.0f, 2.0f);
    Vector2 instr2Pos = { (winW - instr2Size.x) / 2.0f, boxY + boxH + 110.0f };
    DrawTextEx(titleFont, instr2, instr2Pos, 15.0f, 2.0f, LIGHTGRAY);
}

// -------------------- Loading Screen --------------------

void DrawLoadingScreen(float& pacX, Font titleFont) {
    loadingFrame++;
    ClearBackground(YELLOW);

    int boxW = 600;
    int boxH = 80;
    int boxX = (GetScreenHeight() - boxW) / 2;
    int boxY = (GetScreenHeight() - boxH) / 2;
    Rectangle boxRect = { (float)boxX, (float)boxY, (float)boxW, (float)boxH };
    DrawRectangleRounded(boxRect, 0.50f, 12, BLACK);

    int pelletCount = 15;
    int pelletSpacing = 40;
    int pelletY = boxY + boxH / 2;

    for (int i = 0; i < pelletCount; i++) {
        int px = boxX + 60 + i * pelletSpacing;
        if (pacX < px - 10)
            DrawCircle(px, pelletY, 6, WHITE);
    }

    float mouthAngle = (loadingFrame % 20 < 10) ? 45.0f : 10.0f;
    pacX += 4.0f;
    if (pacX > boxX + boxW - 40)
        pacX = boxX + 20;

    DrawCircleSector({ pacX, (float)pelletY }, 24, mouthAngle, 360 - mouthAngle, 40, YELLOW);

    Vector2 textPos = { boxX + boxW / 2.0f - 120, boxY + boxH + 40.0f };
    if ((loadingFrame / 30) % 2 == 0)
        DrawTextEx(titleFont, "LOADING...", textPos, 30.0f, 2.0f, BLACK);
}

// -------------------- Exit Screen --------------------
void DrawExitScreen(int winW, int winH, Font titleFont, int& gameOverTimer) {
    static float slideY = -100.0f;
    if (gameResetFlag) {
        slideY = -100.0f;
        gameResetFlag = false;
    }

    float targetY = winH / 2.0f - 50.0f;
    if (slideY < targetY) slideY += 5.0f;
    if (slideY > targetY) slideY = targetY;

    const char* msg = "GAME OVER";
    int fontSize = 50;
    Vector2 textSize = MeasureTextEx(titleFont, msg, (float)fontSize, 2.0f);
    Vector2 textPos = { (winW - textSize.x) / 2.0f, slideY };
    DrawTextEx(titleFont, msg, textPos, (float)fontSize, 2.0f, RED);
}

// -------------------- Reset Game --------------------
void resetGame(Map& maze, Pacman& pac, RedGhost& red, PinkGhost& pink, OrangeGhost& orange, BlueGhost& blue,
    vector<string>& originalLayout, int tileSize, int& frightenedTimer,
    int& globalFrames, int& waveTimer, bool& scatterMode,
    ReleaseInfo& redRelease, ReleaseInfo& pinkRelease, ReleaseInfo& orangeRelease, ReleaseInfo& blueRelease,
    int& pacEnergizerTimer)
{
    maze.layout = originalLayout;
    maze.coins = CoinList();
    for (int y = 0; y < maze.rows; y++)
        for (int x = 0; x < maze.cols; x++)
            if (maze.layout[y][x] == '.') maze.coins.addCoin(x, y);

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

    red.position = { 10.f * tileSize, 8.f * tileSize };
    pink.position = { 9.f * tileSize, 9.f * tileSize };
    orange.position = { 10.f * tileSize, 9.f * tileSize };
    blue.position = { 11.f * tileSize, 9.f * tileSize };
    red.frightened_mode = pink.frightened_mode = orange.frightened_mode = blue.frightened_mode = 0;

    redRelease.state = R_ACTIVE;
    pinkRelease.state = orangeRelease.state = blueRelease.state = R_IN_CAGE;
    pinkRelease.timer = orangeRelease.timer = blueRelease.timer = 0;

    frightenedTimer = 0;
    pacEnergizerTimer = 0;
    globalFrames = 0;
    waveTimer = 0;
    scatterMode = true;
    gameResetFlag = true;
}