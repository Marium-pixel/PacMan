#include "raylib.h"
#include "GameConstants.h"
#include "Pacman.h"
#include "Ghost.h"
#include "Map.h"
#include "Menu.h"
#include "Highscore.h"

#include<iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <array>
#include <cmath>
#include <queue>
#include <fstream>
#include <sstream>

using namespace std;
using namespace GameConstants; 

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

    // ---------------------- FIXED AUDIO ----------------------
    InitWindow(winW, winH, "PacMaze - Raylib Grid Integrated");

    InitAudioDevice();              // MUST BE BEFORE ANY SOUND LOAD
    SetMasterVolume(1.0f);

    Sound gameOverSound = LoadSound("C:/Users/ARMEEN_AZAM/Desktop/coal lab tasks/Ray/x64/Debug/gameover2.wav");
    //-----------------------------------------------------------

    SetTargetFPS(60);

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

    bool savedGameScore = false;

    Font titleFont = LoadFont("pacman_font.ttf");  // Replace with your font file
    Font instructionFont = LoadFont("instruction.ttf");
    vector<HighscoreEntry> highscores;
    LoadHighscores(highscores);

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
                if (!savedGameScore) {
                    // Use playerName if set, otherwise fallback to "ANON"
                    string nameToSave = playerName.empty() ? string("ANON") : playerName;
                    highscores.push_back({ nameToSave, pac.score });
                    SaveHighscores(highscores);
                    savedGameScore = true;
                }
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
                highscores.push_back({ playerName, pac.score });
                SaveHighscores(highscores);
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


        // -------------------- HIGH SCORE SCREEN --------------------
        if (currentState == STATE_HIGHSCORE) {
            DrawHighscoreScreen(winW, winH, titleFont, highscores, playerName);

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
                currentState = STATE_MENU;
                selectedOption = MENU_PLAY;
            }

            EndDrawing();
            continue;
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

            // 1?? Pac-Man moves
            CoinList coins;
            pac.updatePacMan(maze, coins);

            // 2?? Check for large pellet

            // -------------------- Update ghosts (call in your main loop) --------------------

// Red
            if (red.frightened_mode == 2) {
                // eyes-eaten state ? backtrack to gate using BFS
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
                // Energizer finished ? reset frightened mode if needed
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

        // ? ADDED — proper flashing
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
