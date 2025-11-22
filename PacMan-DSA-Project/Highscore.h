#pragma once
#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include <vector>
#include <string>
#include <fstream>
#include "raylib.h"
#include<string>
#include <vector>

using namespace std;

struct HighscoreEntry {
    string name;
    int score;
};


void Merge(vector<HighscoreEntry>& hs, int left, int mid, int right);
void MergeSortHighscores(vector<HighscoreEntry>& hs, int left, int right);

// File I/O
void LoadHighscores(vector<HighscoreEntry>& highscores);
void SaveHighscores(const vector<HighscoreEntry>& highscores);

// Sorting
void SortHighscores(vector<HighscoreEntry>& hs);

// Draw
void DrawHighscoreScreen(int winW, int winH, Font font,
    vector<HighscoreEntry>& highscores,
    const std::string& currentPlayer);

#endif // HIGHSCORE_H
