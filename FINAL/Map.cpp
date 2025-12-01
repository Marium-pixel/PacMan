#include "map.h"
#include <algorithm>
using namespace std;

// -------------------- CoinList Methods --------------------
void CoinList::addCoin(int x, int y) {
    CoinNode* newCoin = new CoinNode{ x, y, head };
    head = newCoin;
}

void CoinList::drawCoins(int tileSize) {
    CoinNode* curr = head;
    while (curr) {
        int cx = curr->x * tileSize + tileSize / 2;
        int cy = curr->y * tileSize + tileSize / 2;
        DrawCircle(cx, cy, tileSize * 0.12f, ORANGE);
        curr = curr->next;
    }
}

bool CoinList::eatCoinAt(int gridX, int gridY) {
    CoinNode* curr = head;
    CoinNode* prev = nullptr;

    while (curr) {
        if (curr->x == gridX && curr->y == gridY) {
            if (prev) prev->next = curr->next;
            else head = curr->next;

            delete curr;
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    return false;
}

// -------------------- Map Methods --------------------
Map::Map(vector<string> mapLayout, int tSize)
    : layout(mapLayout), tileSize(tSize)
{
    isHard = false;    // default
    rows = layout.size();
    cols = layout[0].size();


    // Add coins
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (layout[y][x] == '.')
                coins.addCoin(x, y);
        }
    }

    if (isHard) {
       for (int y = 0; y < rows; y++)
           for (int x = 0; x < cols; x++)
               if (layout[y][x] == 'O')
                   coins.addCoin(x, y); // Replace large pellets with normal floor
    }

    // Mystery Power-Ups (manually)
    mysteryPowerUps = { {16,1}, {3,5}, {4,19}, {15,13}, {10,15}, {6,13} };

    buildAdjList();
}

// Build adjacency list for BFS / ghost pathfinding
void Map::buildAdjList() {
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

// Draw the map
void Map::Draw() {
    Color floorColor = isHard ? GetColor(0x001A26FF) : BLACK;
    Color wallColor = isHard ? GetColor(0x00C8FFFF) : DARKBLUE;

   

    // Ghost home box
    Color ghostBoxColor = isHard ? Color{ 20,0,0,255 } : Color{ 15,15,15,255 };


    // Walkable background
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (layout[y][x] != '#') {
                DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, floorColor);
            }
        }
    }

    // Walls Outline
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (layout[y][x] == '#') {
                int px = x * tileSize;
                int py = y * tileSize;

                if (y == 0 || layout[y - 1][x] != '#') DrawLineEx({ (float)px,(float)py }, { (float)(px + tileSize),(float)py }, 3, wallColor);
                if (y == rows - 1 || layout[y + 1][x] != '#') DrawLineEx({ (float)px,(float)(py + tileSize) }, { (float)(px + tileSize),(float)(py + tileSize) }, 3, wallColor);
                if (x == 0 || layout[y][x - 1] != '#') DrawLineEx({ (float)px,(float)py }, { (float)px,(float)(py + tileSize) }, 3, wallColor);
                if (x == cols - 1 || layout[y][x + 1] != '#') DrawLineEx({ (float)(px + tileSize),(float)py }, { (float)(px + tileSize),(float)(py + tileSize) }, 3, wallColor);
            }
        }
    }

    // Ghost Home Box
    int gxmin = cols, gxmax = -1, gymin = rows, gymax = -1;
    for (int y = 0; y < rows; y++)
        for (int x = 0; x < cols; x++)
            if (layout[y][x] == 'G') {
                gxmin = min(gxmin, x);
                gxmax = max(gxmax, x);
                gymin = min(gymin, y);
                gymax = max(gymax, y);
            }

    if (gxmax >= gxmin && gymax >= gymin) {
        int left = gxmin * tileSize;
        int top = gymin * tileSize;
        int w = (gxmax - gxmin + 1) * tileSize;
        int h = (gymax - gymin + 1) * tileSize;

        DrawRectangle(left, top, w, h, ghostBoxColor);
        DrawRectangleLinesEx({ (float)left,(float)top,(float)w,(float)h }, 2, wallColor);
    }

    // Coins
    coins.drawCoins(tileSize);

    // Large Pellets (skip in hard mode)

    if (!isHard) {
        for (int y = 0; y < rows; y++)
            for (int x = 0; x < cols; x++)
                if (layout[y][x] == 'O') {
                    int cx = x * tileSize + tileSize / 2;
                    int cy = y * tileSize + tileSize / 2;
                    DrawCircle(cx, cy, tileSize * 0.3f, ORANGE);
                }
    }

   

    // Gate
    int gateX = 10 * tileSize;
    int gateY = 8 * tileSize;
    DrawLineEx({ (float)gateX,(float)gateY }, { (float)(gateX + tileSize),(float)gateY }, 3.5f, SKYBLUE);

    // Mystery Power-Ups
    for (auto& tile : mysteryPowerUps) {
        int cx = tile.first * tileSize + tileSize / 2;
        int cy = tile.second * tileSize + tileSize / 2;
        DrawCircle(cx, cy, tileSize * 0.3f, PURPLE);
        int fontSize = tileSize / 2;
        DrawText("?", cx - fontSize / 4, cy - fontSize / 2, fontSize, WHITE);
    }
}

// Check if a tile is a wall
bool Map::isWall(int gx, int gy) {
    if (gx < 0 || gx >= cols || gy < 0 || gy >= rows) return true;
    char c = layout[gy][gx];
    if (c == '#' || (gy == 9 && gx == 10) || c == 'G') return true;
    return false;
}

void Map::eatLargePelletAt(int gx, int gy) {
    if (gx >= 0 && gx < cols && gy >= 0 && gy < rows && layout[gy][gx] == 'O') layout[gy][gx] = ' ';
}
