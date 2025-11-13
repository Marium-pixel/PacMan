//#include "raylib.h"
//#include <iostream>
//#include <vector>
//#include <unordered_map>
//#include <string>
//using namespace std;
//
//// ---------------- Linked List for Coins ----------------
//struct CoinNode {
//    int x, y;
//    CoinNode* next;
//};
//
//class CoinList {
//public:
//    CoinNode* head = nullptr;
//
//    void addCoin(int x, int y) {
//        CoinNode* newCoin = new CoinNode{ x, y, head };
//        head = newCoin;
//    }
//
//    void drawCoins(int tileSize) {
//        CoinNode* curr = head;
//        while (curr) {
//            // draw pellet as a small white dot
//            int cx = curr->x * tileSize + tileSize / 2;
//            int cy = curr->y * tileSize + tileSize / 2;
//            DrawCircle(cx, cy, tileSize * 0.08f, WHITE);
//            curr = curr->next;
//        }
//    }
//};
//
//// ---------------- Maze Graph (Adjacency List) ----------------
//struct Node {
//    int x, y;
//    vector<pair<int, int>> neighbors;
//};
//
//class MazeGraph {
//public:
//    unordered_map<string, Node> graph;
//    vector<string> layout;
//    int rows, cols, tileSize;
//    CoinList coins;
//
//    MazeGraph(vector<string> mapLayout, int tSize)
//        : layout(mapLayout), tileSize(tSize) {
//        rows = layout.size();
//        cols = layout[0].size();
//        buildGraph();
//    }
//
//    string key(int x, int y) { return to_string(x) + "," + to_string(y); }
//
//    void buildGraph() {
//        for (int y = 0; y < rows; y++) {
//            for (int x = 0; x < cols; x++) {
//                char cell = layout[y][x];
//                if (cell == '#') continue; // Wall = not walkable
//
//                Node node = { x, y, {} };
//
//                // Add neighbors (4-directional)
//                if (y > 0 && layout[y - 1][x] != '#')
//                    node.neighbors.push_back({ x, y - 1 });
//                if (y < rows - 1 && layout[y + 1][x] != '#')
//                    node.neighbors.push_back({ x, y + 1 });
//                if (x > 0 && layout[y][x - 1] != '#')
//                    node.neighbors.push_back({ x - 1, y });
//                if (x < cols - 1 && layout[y][x + 1] != '#')
//                    node.neighbors.push_back({ x + 1, y });
//
//                graph[key(x, y)] = node;
//
//                if (cell == '.') coins.addCoin(x, y); // Pellet cell
//            }
//        }
//    }
//
//    void draw() {
//        // Draw floor first (dark gray corridors)
//        for (int y = 0; y < rows; y++) {
//            for (int x = 0; x < cols; x++) {
//                char cell = layout[y][x];
//                int px = x * tileSize;
//                int py = y * tileSize;
//
//                if (cell == '#') {
//                    // skip here; walls drawn later
//                    continue;
//                }
//                else {
//                    // floor color (slightly lighter than black)
//                    DrawRectangle(px, py, tileSize, tileSize, BLACK);
//                }
//            }
//        }
//
//
//        //int pad = 0;
//        //for (int y = 0; y < rows; y++) {
//        //    for (int x = 0; x < cols; x++) {
//        //        if (layout[y][x] == '#') {
//        //            int px = x * tileSize + pad;
//        //            int py = y * tileSize + pad;
//        //            int w = tileSize;
//        //            int h = tileSize;
//
//        //            // Check adjacent cells and draw outlines only where needed
//        //            // Top edge
//        //            if (y == 0 || layout[y - 1][x] != '#')
//        //                DrawLine(px, py, px + w, py, DARKBLUE);
//
//        //            // Bottom edge
//        //            if (y == rows - 1 || layout[y + 1][x] != '#')
//        //                DrawLine(px, py + h, px + w, py + h, DARKBLUE);
//
//        //            // Left edge
//        //            if (x == 0 || layout[y][x - 1] != '#')
//        //                DrawLine(px, py, px, py + h, DARKBLUE);
//
//        //            // Right edge
//        //            if (x == cols - 1 || layout[y][x + 1] != '#')
//        //                DrawLine(px + w, py, px + w, py + h, DARKBLUE);
//        //        }
//        //    }
//        //}
//
//
//        // Draw the ghost home as an outlined rectangle (use 'G' in layout)
//        // We'll draw a thin dark-blue outline (like the image) and black interior.
//        // The G cells are considered interior; find bounding box to draw one coherent home.
//        int gxmin = cols, gxmax = -1, gymin = rows, gymax = -1;
//        for (int y = 0; y < rows; y++) {
//            for (int x = 0; x < cols; x++) {
//                if (layout[y][x] == 'G') {
//                    gxmin = min(gxmin, x);
//                    gxmax = max(gxmax, x);
//                    gymin = min(gymin, y);
//                    gymax = max(gymax, y);
//                }
//            }
//        }
//        if (gxmax >= gxmin && gymax >= gymin) {
//            int left = gxmin * tileSize + pad;
//            int top = gymin * tileSize + pad;
//            int w = (gxmax - gxmin + 1) * tileSize - pad * 2;
//            int h = (gymax - gymin + 1) * tileSize - pad * 2;
//            // outline in dark blue, interior darker to show gate opening
//            DrawRectangle(left, top, w, h, Color{ 12, 12, 12, 255 });
//            DrawRectangleLines(left, top, w, h, DARKBLUE);
//        }
//
//        // Draw pellets (coins)
//        coins.drawCoins(tileSize);
//
//        // Draw power pellets locations explicitly (big white circles)
//        for (int y = 0; y < rows; y++) {
//            for (int x = 0; x < cols; x++) {
//                if (layout[y][x] == 'O') {
//                    int cx = x * tileSize + tileSize / 2;
//                    int cy = y * tileSize + tileSize / 2;
//                    DrawCircle(cx, cy, tileSize * 0.25f, WHITE);
//                }
//            }
//        }
//
//        // Optional: draw subtle grid lines (very faint)
//        for (int y = 0; y <= rows; y++) {
//            DrawLine(0, y * tileSize, cols * tileSize, y * tileSize, Fade(BLACK, 0.0f));
//        }
//        for (int x = 0; x <= cols; x++) {
//            DrawLine(x * tileSize, 0, x * tileSize, rows * tileSize, Fade(BLACK, 0.0f));
//        }
//    }
//};
//
//// ---------------- Main ----------------
//int main() {
//    // === Carefully crafted layout (28 columns x 31 rows)
//    // '#' = wall block (drawn as separated dark-blue tiles)
//    // '.' = pellet (small white dot)
//    // 'O' = power pellet (big white)
//    // 'G' = ghost home interior
//    //vector<string> mazeLayout = {
//    //    "############################",
//    //    "#............##............#",
//    //    "#.####.#####.##.#####.####.#",
//    //    "#.####.#####.##.#####.####.#",
//    //    "#.####.#####.##.#####.####.#",
//    //    "#..........................#",
//    //    "#.####.##.########.##.####.#",
//    //    "#.####.##.########.##.####.#",
//    //    "#......##....##....##......#",
//    //    "######.##### ## #####.######",
//    //    "######.##### ## #####.######",
//    //    "######.##          ##.######",
//    //    "######.## ######## ##.######",
//    //    "######.## #GGGGGG# ##.######",
//    //    "######.## #GGGGGG# ##.######",
//    //    "######.## ######## ##.######",
//    //    "######.##          ##.######",
//    //    "######.## ######## ##.######",
//    //    "######.## ######## ##.######",
//    //    "#............##............#",
//    //    "#.####.#####.##.#####.####.#",
//    //    "#.####.#####.##.#####.####.#",
//    //    "#...##................##...#",
//    //    "###.##.##.########.##.##.###",
//    //    "###.##.##.########.##.##.###",
//    //    "#......##....##....##......#",
//    //    "#.##########.##.##########.#",
//    //    "#..........................#",
//    //    "############################",
//    //    "############################" // extra bottom padding row for aspect balance
//    //};
//
//    //// Increase tileSize to make the window larger and arcade-like.
//    //// choose tileSize such that walls have visible gaps; adjust for your screen if needed.
//    //int tileSize = 18;
//
//    //MazeGraph maze(mazeLayout, tileSize);
//
//    //// Window size matches the maze pixel dimensions
//    //int winW = maze.cols * tileSize;
//    //int winH = maze.rows * tileSize;
//    //InitWindow(winW, winH, "PacMaze - Arcade Style");
//    //SetTargetFPS(60);
//
//    //while (!WindowShouldClose()) {
//    //    BeginDrawing();
//    //    ClearBackground(BLACK);
//
//    //    maze.draw();
//
//    //    // small HUD/footer (lives/cherries placeholders)
//    //    DrawText("1UP    HIGH SCORE", 8, 6, 14, WHITE);
//
//    //    EndDrawing();
//    //}
//
//    //CloseWindow();
//
//
//    const int tileSize = 24;
//    const int rows = 15;
//    const int cols = 19;
//
//    InitWindow(cols * tileSize, rows * tileSize, "Pac-Man Grid Outline Example");
//    SetTargetFPS(60);
//
//    // Sample Pac-Man-style map layout
//    std::vector<std::string> layout = {
//        "###################",
//        "#........#........#",
//        "#.###.###.#.###.##",
//        "#o###.###.#.###.o#",
//        "#.................#",
//        "###.#.#####.#.###.#",
//        "###.#...#...#.###.#",
//        "###.###.#.###.###.#",
//        "#.........G........#",
//        "###################"
//    };
//
//    while (!WindowShouldClose()) {
//        BeginDrawing();
//        ClearBackground(BLACK);
//
//        // Draw maze grid
//        for (int y = 0; y < rows; y++) {
//            for (int x = 0; x < cols; x++) {
//                char tile = layout[y][x];
//                int px = x * tileSize;
//                int py = y * tileSize;
//
//                // WALL outline drawing (connected)
//                if (tile == '#') {
//                    // Draw outline only on exposed sides
//                    if (y == 0 || layout[y - 1][x] != '#')
//                        DrawLineEx({ (float)px, (float)py }, { (float)(px + tileSize), (float)py }, 3, DARKBLUE);
//                    if (y == rows - 1 || layout[y + 1][x] != '#')
//                        DrawLineEx({ (float)px, (float)(py + tileSize) }, { (float)(px + tileSize), (float)(py + tileSize) }, 3, DARKBLUE);
//                    if (x == 0 || layout[y][x - 1] != '#')
//                        DrawLineEx({ (float)px, (float)py }, { (float)px, (float)(py + tileSize) }, 3, DARKBLUE);
//                    if (x == cols - 1 || layout[y][x + 1] != '#')
//                        DrawLineEx({ (float)(px + tileSize), (float)py }, { (float)(px + tileSize), (float)(py + tileSize) }, 3, DARKBLUE);
//                }
//
//                // PELLETS
//                else if (tile == '.') {
//                    DrawCircle(px + tileSize / 2, py + tileSize / 2, 3, YELLOW);
//                }
//
//                // BIG PELLETS
//                else if (tile == 'o') {
//                    DrawCircle(px + tileSize / 2, py + tileSize / 2, 6, YELLOW);
//                }
//
//                // GHOST HOME
//                else if (tile == 'G') {
//                    DrawRectangle(px + 4, py + 4, tileSize - 8, tileSize - 8, MAROON);
//                }
//            }
//        }
//
//        EndDrawing();
//    }
//    CloseWindow();
//
//    return 0;
//}
