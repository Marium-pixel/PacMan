#include "Difficulty.h"
#include "raylib.h"

void DrawLevelSelectScreen(int winW, int winH, Font font, int selected)
{
    ClearBackground(BLACK);

    const char* title = "SELECT DIFFICULTY";
    int size = 40;

    Vector2 tSize = MeasureTextEx(font, title, size, 4);
    DrawTextEx(font, title,
        { (winW - tSize.x) / 2.0f, 100 },
        size, 4, YELLOW);

    // Options
    const char* options[2] = { "EASY MODE", "HARD MODE" };

    for (int i = 0; i < 2; i++) {
        Color c = (i == selected ? RED : WHITE);

        Vector2 s = MeasureTextEx(font, options[i], 30, 2);
        DrawTextEx(font, options[i],
            { (winW - s.x) / 2.0f, (float)(220 + i * 60) },
            30, 2, c);

    }

    DrawText("Use UP/DOWN and ENTER", winW / 2 - 160, winH - 80, 20, GRAY);
}