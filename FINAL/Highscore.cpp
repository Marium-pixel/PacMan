#include "Highscore.h"

#include <algorithm>
#include <iostream>

using namespace std;

// -------------------- Merge Sort --------------------
void Merge(vector<HighscoreEntry>& hs, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    vector<HighscoreEntry> L(n1), R(n2);

    for (int i = 0; i < n1; i++) L[i] = hs[left + i];
    for (int i = 0; i < n2; i++) R[i] = hs[mid + 1 + i];

    int i = 0, j = 0, k = left;

    while (i < n1 && j < n2) {
        if (L[i].score >= R[j].score) { // Descending
            hs[k++] = L[i++];
        }
        else {
            hs[k++] = R[j++];
        }
    }
    while (i < n1) hs[k++] = L[i++];
    while (j < n2) hs[k++] = R[j++];
}

void MergeSortHighscores(vector<HighscoreEntry>& hs, int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    MergeSortHighscores(hs, left, mid);
    MergeSortHighscores(hs, mid + 1, right);
    Merge(hs, left, mid, right);
}

void SortHighscores(vector<HighscoreEntry>& hs) {
    if (hs.empty()) return;
    MergeSortHighscores(hs, 0, hs.size() - 1);
}

// -------------------- Load / Save --------------------
void LoadHighscores(vector<HighscoreEntry>& highscores) {
    highscores.clear();
    ifstream file("highscores.txt");
    if (!file.is_open()) {
        highscores.push_back({ "AAA", 3000 });
        highscores.push_back({ "BBB", 1500 });
        highscores.push_back({ "CCC", 900 });
        return;
    }

    string name;
    int score;
    while (file >> name >> score) {
        highscores.push_back({ name, score });
    }
    file.close();
}

void SaveHighscores(const vector<HighscoreEntry>& highscores) {
    ofstream file("highscores.txt", ios::trunc);
    for (const auto& h : highscores) {
        file << h.name << " " << h.score << "\n";
    }
    file.close();
}

// -------------------- Draw Highscores --------------------
void DrawHighscoreScreen(int winW, int winH, Font font,
    vector<HighscoreEntry>& highscores,
    const string& currentPlayer)
{
    ClearBackground(BLACK);

    const char* title = "HIGHSCORES";
    int titleFontSize = 40;
    Vector2 titleSize = MeasureTextEx(font, title, (float)titleFontSize, 4.0f);
    Vector2 titlePos = { (winW - titleSize.x) / 2.0f, 100.0f };
    DrawTextEx(font, title, titlePos, (float)titleFontSize, 4.0f, YELLOW);

    if (!highscores.empty())
        MergeSortHighscores(highscores, 0, highscores.size() - 1);

    int scoreFontSize = 25;
    float startY = 200.0f;
    float lineHeight = 50.0f;

    for (int i = 0; i < 3 && i < highscores.size(); i++) {
        string line = to_string(i + 1) + ". " + highscores[i].name + " - " + to_string(highscores[i].score);
        Vector2 lineSize = MeasureTextEx(font, line.c_str(), (float)scoreFontSize, 2.0f);
        Vector2 linePos = { (winW - lineSize.x) / 2.0f, startY + i * lineHeight };
        DrawTextEx(font, line.c_str(), linePos, (float)scoreFontSize, 2.0f, WHITE);
    }

    int playerRank = -1;
    for (int i = 0; i < highscores.size(); i++) {
        if (highscores[i].name == currentPlayer) {
            playerRank = i + 1;
            break;
        }
    }

    if (playerRank != -1) {
        string youLine = "You: " + to_string(playerRank) + " " + highscores[playerRank - 1].name + " - " + to_string(highscores[playerRank - 1].score);
        Vector2 youSize = MeasureTextEx(font, youLine.c_str(), (float)scoreFontSize, 2.0f);
        Vector2 youPos = { (winW - youSize.x) / 2.0f, startY + 3 * lineHeight + 20 };
        DrawTextEx(font, youLine.c_str(), youPos, (float)scoreFontSize, 2.0f, GREEN);
    }

    const char* prompt = "Press ENTER or ESCAPE to return";
    Vector2 promptSize = MeasureTextEx(font, prompt, 15, 2.0f);
    Vector2 promptPos = { (winW - promptSize.x) / 2.0f, winH - 60.0f };
    DrawTextEx(font, prompt, promptPos, 15, 2.0f, LIGHTGRAY);
}
