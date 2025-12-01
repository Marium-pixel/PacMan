#pragma once
#ifndef MAP_H
 #define MAP_H
#include "raylib.h"
#include "GameConstants.h"
#include <vector>
#include <string>
#include <unordered_map>
  
using namespace std;

// -------------------- Linked List for Coins --------------------
struct CoinNode {
    int x, y;
    CoinNode* next;
};

class CoinList {
public:
    CoinNode* head = nullptr;

    void addCoin(int x, int y);
    void drawCoins(int tileSize);
    bool eatCoinAt(int gridX, int gridY);
};

// -------------------- Map Class --------------------
class Map {
public:
    vector<string> layout;
    int rows, cols, tileSize;
    CoinList coins;
	vector<pair<int, int>> mysteryPowerUps;  //(col, row)
    unordered_map<int, vector<int>> adjList;
    bool isHard;

    Map(vector<string> mapLayout, int tSize);

    void buildAdjList();
    void Draw();
    bool isWall(int gx, int gy);
    void eatLargePelletAt(int gx, int gy);

    //void setHardMode(bool h);  // <-- NEW
};

#endif // MAP_H
