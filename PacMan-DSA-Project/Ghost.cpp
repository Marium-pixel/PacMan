#include "Ghost.h"
#include "Map.h"
#include "Pacman.h"

// -------------------- Ghost Base Class --------------------
Ghost::Ghost(int gx, int gy, int i_id, Texture2D tex, int tileSize)
    : id(i_id), direction(0), frightened_mode(0), animation_timer(0), texture(tex)
{
    position = { (float)gx * tileSize, (float)gy * tileSize };
    cageX = gx;
    cageY = gy;
    gateX = gx;
    gateY = gy - 1;
    eyesTargetX = gateX;
    eyesTargetY = gateY;
}

void Ghost::moveToGate(Map& map, int tileSize) {
    if (frightened_mode != 2) return; // only eyes mode

    int gx = (int)(position.x / tileSize);
    int gy = (int)(position.y / tileSize);

    if (gx < eyesTargetX && !map.isWall(gx + 1, gy)) position.x += speed;
    else if (gx > eyesTargetX && !map.isWall(gx - 1, gy)) position.x -= speed;
    else if (gy < eyesTargetY && !map.isWall(gx, gy + 1)) position.y += speed;
    else if (gy > eyesTargetY && !map.isWall(gx, gy - 1)) position.y -= speed;

    gx = (int)(position.x / tileSize);
    gy = (int)(position.y / tileSize);
    if (gx == eyesTargetX && gy == eyesTargetY) {
        frightened_mode = 0;
        releaseState = R_EXITING_GATE;
    }
}

void Ghost::draw(bool i_flash, int tileSize) {
    int body_frame = (animation_timer / GHOST_ANIMATION_SPEED) % GHOST_ANIMATION_FRAMES;
    Rectangle srcBody = { body_frame * 16.0f, 0.0f, 16.0f, 16.0f };
    Rectangle dstRect = { position.x, position.y, (float)tileSize, (float)tileSize };
    Vector2 origin = { 0.0f, 0.0f };
    Color bodyColor = WHITE;

    switch (id) {
    case 0: bodyColor = RED; break;
    case 1: bodyColor = Color{ 255,182,255,255 }; break;
    case 2: bodyColor = Color{ 0,255,255,255 }; break;
    case 3: bodyColor = Color{ 255,182,85,255 }; break;
    }

    Rectangle srcFace;
    if (frightened_mode == 0) {
        srcFace = { (float)(CELL_SIZE * direction), (float)CELL_SIZE, (float)CELL_SIZE, (float)CELL_SIZE };
        DrawTexturePro(texture, srcBody, dstRect, origin, 0.0f, bodyColor);
        DrawTexturePro(texture, srcFace, dstRect, origin, 0.0f, WHITE);
    }
    else if (frightened_mode == 1) {
        Color frightenedBlue = Color{ 36,36,255,255 };
        Color faceColor = WHITE;
        srcFace = { (float)(4 * CELL_SIZE), (float)CELL_SIZE, (float)CELL_SIZE, (float)CELL_SIZE };
        if (i_flash && (body_frame % 2) == 0) { bodyColor = WHITE; faceColor = Color{ 255,0,0,255 }; }
        else bodyColor = frightenedBlue;

        DrawTexturePro(texture, srcBody, dstRect, origin, 0.0f, bodyColor);
        DrawTexturePro(texture, srcFace, dstRect, origin, 0.0f, faceColor);
    }
    else if (frightened_mode == 2) {
        srcFace = { (float)(CELL_SIZE * direction), (float)(2 * CELL_SIZE), (float)CELL_SIZE, (float)CELL_SIZE };
        DrawTexturePro(texture, srcFace, dstRect, origin, 0.0f, WHITE);
    }

    animation_timer = (animation_timer + 1) % (GHOST_ANIMATION_FRAMES * GHOST_ANIMATION_SPEED);
}

void Ghost::updateReleaseState(Map& map, float tileSize) {
    if (frightened_mode == 2) {
        moveToGate(map, tileSize);
        return;
    }
    switch (releaseState) {
    case R_IN_CAGE:
        if (position.y > gateY * tileSize) position.y -= speed;
        else releaseState = R_EXITING_GATE;
        break;
    case R_EXITING_GATE:
        if (position.y > (gateY - 1) * tileSize) position.y -= speed;
        else releaseState = R_ACTIVE;
        break;
    case R_ACTIVE: break;
    }
}

// -------------------- RedGhost --------------------
RedGhost::RedGhost(int gx, int gy, int id, Texture2D tex, int tile)
    : Ghost(gx, gy, id, tex, tile) {
}

void RedGhost::update(Pacman& p, Map& m, int tileSize, bool scatterMode) {
    if (frightened_mode == 1) { fleeFromPacman(*this, p, m, tileSize, 1.0f); return; }
    if (scatterMode) scatterToCorner(*this, m, tileSize);
    else chasePacmanToTarget(*this, p, m, tileSize);
}
void RedGhost::draw(bool debug, int tileSize) { Ghost::draw(debug, tileSize); }

// -------------------- PinkGhost --------------------
PinkGhost::PinkGhost(int gx, int gy, int id, Texture2D tex, int tile)
    : Ghost(gx, gy, id, tex, tile) {
}

void PinkGhost::scatterToTopLeft(Map& m, int tileSize) {
    int tx = -1, ty = -1;

    // find top-left walkable tile
    for (int row = 0; row < m.rows; row++) {
        for (int col = 0; col < m.cols; col++) {
            if (!m.isWall(col, row)) {
                tx = col;
                ty = row;
                break;
            }
        }
        if (tx != -1) break;
    };

    if (tx == -1) { tx = 1; ty = 1; } // fallback

    navigateGhostToTile(*this, m, tileSize, tx, ty, 1.5f);
}

void PinkGhost::chaseTarget(Pacman& p, Map& m, int tileSize) {
    int tx = (int)((p.x + tileSize / 2) / tileSize);
    int ty = (int)((p.y + tileSize / 2) / tileSize);

    switch (p.direction) {
    case Pacman::UP:    ty -= 4; break;
    case Pacman::DOWN:  ty += 4; break;
    case Pacman::LEFT:  tx -= 4; break;
    case Pacman::RIGHT: tx += 4; break;
    default: break;
    }

    navigateGhostToTile(*this, m, tileSize, tx, ty, 1.5f);
}

void PinkGhost::update(Pacman& p, Map& m, int tileSize, bool scatterMode) {
    if (frightened_mode == 1) {
        fleeFromPacman(*this, p, m, tileSize, 1.0f);  // Adjust speed as needed
        return;
    }
    if (scatterMode)
        scatterToTopLeft(m, tileSize);
    else
        chaseTarget(p, m, tileSize);
}
void PinkGhost::draw(bool debug, int tileSize) { Ghost::draw(debug, tileSize); }


// -------------------- OrangeGhost --------------------
OrangeGhost::OrangeGhost(int gx, int gy, int id, Texture2D tex, int tile)
    : Ghost(gx, gy, id, tex, tile) {
}

// Scatter to bottom-left walkable tile
void OrangeGhost::scatterToBottomLeft(Map& m, int tileSize) {
    int tx = -1, ty = -1;

    // search bottom-left: start from last row upwards, leftmost columns
    for (int row = m.rows - 1; row >= 0; row--) {
        for (int col = 0; col < m.cols; col++) {
            if (!m.isWall(col, row)) {
                tx = col;
                ty = row;
                break;
            }
        }
        if (tx != -1) break;
    }

    if (tx == -1) { tx = 1; ty = m.rows - 2; } // fallback
    navigateGhostToTile(*this, m, tileSize, tx, ty, 1.5f);
}

// Chase 2 tiles in front of Pacman
void OrangeGhost ::chaseTarget(Pacman& p, Map& m, int tileSize) {
    int tx = (int)((p.x + tileSize / 2) / tileSize);
    int ty = (int)((p.y + tileSize / 2) / tileSize);

    // 2 tiles in front
    switch (p.direction) {
    case Pacman::UP:    ty -= 2; break;
    case Pacman::DOWN:  ty += 2; break;
    case Pacman::LEFT:  tx -= 2; break;
    case Pacman::RIGHT: tx += 2; break;
    default: break;
    }
    // Check distance to pacman (Manhattan)
    int gx = (int)((position.x + tileSize / 2) / tileSize);
    int gy = (int)((position.y + tileSize / 2) / tileSize);
    int manhDist = abs(gx - tx) + abs(gy - ty);

    if (manhDist <= 3) {
        // too close ? switch to scatter temporarily
        scatterToBottomLeft(m, tileSize);
    }
    else {
        navigateGhostToTile(*this, m, tileSize, tx, ty, 1.5f);
    }
}

void OrangeGhost::update(Pacman& p, Map& m, int tileSize, bool scatterMode) {
    if (frightened_mode == 1) {
        fleeFromPacman(*this, p, m, tileSize, 1.0f);  // Adjust speed as needed
        return;
    }
    if (scatterMode)
        scatterToBottomLeft(m, tileSize);
    else
        chaseTarget(p, m, tileSize);
}

void OrangeGhost:: draw(bool debug, int tileSize) {
    Ghost::draw(debug, tileSize);
}


// -------------------- BlueGhost --------------------
BlueGhost::BlueGhost(int gx, int gy, int id, Texture2D tex, int tile)
    : Ghost(gx, gy, id, tex, tile) {
}

// Scatter to bottom-right corner
void BlueGhost:: scatterToBottomRight(Map& m, int tileSize) {
    int tx = -1, ty = -1;

    for (int row = m.rows - 1; row >= 0; --row) {
        for (int col = m.cols - 1; col >= 0; --col) {
            if (!m.isWall(col, row)) {
                tx = col;
                ty = row;
                break;
            }
        }
        if (tx != -1) break;
    }

    if (tx == -1) { tx = m.cols - 2; ty = m.rows - 2; } // fallback
    navigateGhostToTile(*this, m, tileSize, tx, ty, 1.5f);
}

// Chase target using RedGhost and Pacman
void BlueGhost::chaseTarget(Pacman& p, RedGhost& red, Map& m, int tileSize) {
    // Current tile positions
    float pacX = p.x + tileSize / 2.0f;
    float pacY = p.y + tileSize / 2.0f;
    float redX = red.position.x + tileSize / 2.0f;
    float redY = red.position.y + tileSize / 2.0f;

    // Vector from Red ? Pacman
    float vx = pacX - redX;
    float vy = pacY - redY;

    // Extend vector to grid boundary
    float scaleX = vx > 0 ? (m.cols * tileSize - pacX) / vx : (pacX) / -vx;
    float scaleY = vy > 0 ? (m.rows * tileSize - pacY) / vy : (pacY) / -vy;
    float scale = std::min(scaleX, scaleY); // scale until hitting boundary

    float targetX = pacX + vx * scale;
    float targetY = pacY + vy * scale;

    // Convert to tile coordinates
    int tx = (int)(targetX / tileSize);
    int ty = (int)(targetY / tileSize);

    navigateGhostToTile(*this, m, tileSize, tx, ty, 1.5f);
}

void BlueGhost::update(Pacman& p, RedGhost& red, Map& m, int tileSize, bool scatterMode) {
    if (frightened_mode == 1) {
        fleeFromPacman(*this, p, m, tileSize, 1.0f);  // Adjust speed as needed
        return;
    }

    if (scatterMode) {
        scatterToBottomRight(m, tileSize);
    }
    else {
        chaseTarget(p, red, m, tileSize);
    }
}

void BlueGhost::draw(bool debug, int tileSize) {
    Ghost::draw(debug, tileSize);
}

// -------------------- Functions --------------------
// Implement navigateToTile, backtrackToGate, fleeFromPacman, chasePacmanToTarget, etc.

void navigateToTile(Ghost& g, Map& map, int tileSize, int tx, int ty, float speed) {
    // current ghost tile
    int gx = (int)((g.position.x + tileSize / 2) / tileSize);
    int gy = (int)((g.position.y + tileSize / 2) / tileSize);

    // clamp target
    if (tx < 0) tx = 0; if (tx >= map.cols) tx = map.cols - 1;
    if (ty < 0) ty = 0; if (ty >= map.rows) ty = map.rows - 1;

    // BFS setup
    vector<vector<bool>> visited(map.rows, vector<bool>(map.cols, false));
    vector<vector<pair<int, int>>> parent(map.rows, vector<pair<int, int>>(map.cols, { -1,-1 }));
    queue<pair<int, int>> q;
    q.push({ gy, gx });
    visited[gy][gx] = true;

    int dirs[4][2] = { {-1,0},{1,0},{0,-1},{0,1} }; // up, down, left, right
    bool found = false;

    while (!q.empty() && !found) {
        auto cur = q.front(); q.pop();
        for (auto& d : dirs) {
            int ny = cur.first + d[0];
            int nx = cur.second + d[1];
            if (nx >= 0 && nx < map.cols && ny >= 0 && ny < map.rows &&
                !visited[ny][nx] && !map.isWall(nx, ny))
            {
                visited[ny][nx] = true;
                parent[ny][nx] = cur;
                if (nx == tx && ny == ty) { found = true; break; }
                q.push({ ny, nx });
            }
        }
    }

    // determine next cell to move toward
    pair<int, int> nextCell = { gy, gx };

    if (found) {
        // backtrack from target until the immediate step next to ghost
        pair<int, int> cur = { ty, tx };
        // safety guard in case of malformed parent
        while (parent[cur.first][cur.second] != make_pair(gy, gx)) {
            if (parent[cur.first][cur.second].first == -1) break;
            cur = parent[cur.first][cur.second];
        }
        nextCell = cur;
    }
    else {
        // choose best reachable tile (closest to target by Manhattan) among visited
        int bestDist = INT_MAX;
        pair<int, int> bestCell = { gy, gx };
        for (int r = 0; r < map.rows; ++r) {
            for (int c = 0; c < map.cols; ++c) {
                if (visited[r][c]) {
                    int manh = abs(c - tx) + abs(r - ty);
                    if (manh < bestDist && !(r == gy && c == gx)) {
                        bestDist = manh;
                        bestCell = { r, c };
                    }
                }
            }
        }

        if (!(bestCell.first == gy && bestCell.second == gx)) {
            // backtrack from bestCell to the step next to ghost
            pair<int, int> cur = bestCell;
            while (parent[cur.first][cur.second] != make_pair(gy, gx)) {
                if (parent[cur.first][cur.second].first == -1) break;
                cur = parent[cur.first][cur.second];
            }
            nextCell = cur;
        }
        else {
            // fallback: try any adjacent free tile so ghost doesn't stay stuck
            bool moved = false;
            for (auto& d : dirs) {
                int ny = gy + d[0];
                int nx = gx + d[1];
                if (nx >= 0 && nx < map.cols && ny >= 0 && ny < map.rows && !map.isWall(nx, ny)) {
                    nextCell = { ny, nx };
                    moved = true;
                    break;
                }
            }
            if (!moved) nextCell = { gy, gx }; // no move possible
        }
    }

    // Smooth movement toward nextCell
    float targetX = nextCell.second * tileSize + tileSize / 2.0f;
    float targetY = nextCell.first * tileSize + tileSize / 2.0f;
    float dx = targetX - (g.position.x + tileSize / 2.0f);
    float dy = targetY - (g.position.y + tileSize / 2.0f);
    float len = sqrtf(dx * dx + dy * dy);

    if (len > 0.0001f) {
        dx = dx / len * speed;
        dy = dy / len * speed;
        g.position.x += dx;
        g.position.y += dy;
    }
}

void backtrackToGate(Ghost& g, Map& map, int tileSize, float speed) {
    using Tile = pair<int, int>;
    uintptr_t key = (uintptr_t)(&g);

    // Static caches keyed by ghost pointer address
    static unordered_map<uintptr_t, vector<Tile>> pathCache;    // tiles from ghost -> ... -> gate
    static unordered_map<uintptr_t, size_t> pathIndex;         // next tile index to move to (start at 1)

    // Gate tile (use eyesTarget if present, otherwise use gateX/gateY)
    int gateX = g.eyesTargetX;
    int gateY = g.eyesTargetY;

    // Current ghost tile
    int gx = (int)((g.position.x + tileSize / 2.0f) / tileSize);
    int gy = (int)((g.position.y + tileSize / 2.0f) / tileSize);

    // If ghost already at gate (tile coords) -> finish immediately
    if (gx == gateX && gy == gateY) {
        // ensure full body and start exit process
        g.frightened_mode = 0;
        g.releaseState = R_EXITING_GATE;
        pathCache.erase(key);
        pathIndex.erase(key);
        return;
    }

    // If there's no cached path or the cached path's start doesn't match current tile, recompute
    bool needCompute = false;
    if (pathCache.find(key) == pathCache.end()) needCompute = true;
    else {
        auto& cached = pathCache[key];
        if (cached.empty()) needCompute = true;
        else {
            // cached[0] should be the ghost tile at time of computation; if ghost moved off that tile,
            // recompute to avoid following stale path.
            if (cached.front().first != gy || cached.front().second != gx) needCompute = true;
        }
    }

    if (needCompute) {
        // BFS from gate -> to ghost tile (so parent pointers allow path reconstruction from ghost -> gate)
        int rows = map.rows;
        int cols = map.cols;
        vector<vector<bool>> visited(rows, vector<bool>(cols, false));
        vector<vector<pair<int, int>>> parent(rows, vector<pair<int, int>>(cols, { -1,-1 }));
        queue<pair<int, int>> q;
        q.push({ gateY, gateX });
        visited[gateY][gateX] = true;

        int dirs[4][2] = { {-1,0},{1,0},{0,-1},{0,1} };
        bool found = false;
        while (!q.empty() && !found) {
            auto cur = q.front(); q.pop();
            for (auto& d : dirs) {
                int ny = cur.first + d[0];
                int nx = cur.second + d[1];
                if (nx >= 0 && nx < cols && ny >= 0 && ny < rows && !visited[ny][nx] && !map.isWall(nx, ny)) {
                    visited[ny][nx] = true;
                    parent[ny][nx] = cur;
                    if (ny == gy && nx == gx) { found = true; break; }
                    q.push({ ny, nx });
                }
            }
        }

        vector<Tile> path; // will hold tiles from ghost -> ... -> gate
        if (found) {
            // reconstruct: start at ghost tile, follow parent -> gate
            pair<int, int> cur = { gy, gx };
            path.push_back(cur);
            while (!(cur.first == gateY && cur.second == gateX)) {
                auto p = parent[cur.first][cur.second];
                if (p.first == -1) break; // safety
                cur = p;
                path.push_back(cur);
            }
            // path[0] = ghost tile, path.back() = gate tile
        }
        else {
            // No path found (shouldn't happen in normal maze). Fallback: single-step attempt to adjacent walkable tile.
            path.push_back({ gy, gx });
            // try to find any adjacent free tile that reduces manhattan to gate
            int bestDist = INT_MAX;
            Tile best = { gy, gx };
            for (auto& d : dirs) {
                int ny = gy + d[0], nx = gx + d[1];
                if (nx >= 0 && nx < cols && ny >= 0 && ny < rows && !map.isWall(nx, ny)) {
                    int manh = abs(nx - gateX) + abs(ny - gateY);
                    if (manh < bestDist) { bestDist = manh; best = { ny,nx }; }
                }
            }
            if (!(best.first == gy && best.second == gx)) path.push_back(best);
        }

        // Cache the path and reset index
        pathCache[key] = move(path);
        pathIndex[key] = 1; // next tile to move to (index 1 means move from tile 0 -> tile 1)
    }

    // Move toward next tile in cached path
    auto& pathRef = pathCache[key];
    size_t& idx = pathIndex[key];

    // If path too short or index out of range -> recompute next frame
    if (pathRef.size() < 2 || idx >= pathRef.size()) {
        // fallback: clear and recompute next frame
        pathCache.erase(key);
        pathIndex.erase(key);
        return;
    }

    // Next tile to approach
    Tile nextT = pathRef[idx];
    float targetX = nextT.second * tileSize + tileSize / 2.0f;
    float targetY = nextT.first * tileSize + tileSize / 2.0f;

    // Smooth movement toward center of next tile
    float cx = g.position.x + tileSize / 2.0f;
    float cy = g.position.y + tileSize / 2.0f;
    float dx = targetX - cx;
    float dy = targetY - cy;
    float len = sqrtf(dx * dx + dy * dy);

    if (len < 0.001f) {
        // snapped onto tile center -> advance index
        // place exactly on tile center to avoid jitter
        g.position.x = nextT.second * tileSize;
        g.position.y = nextT.first * tileSize;
        idx++;
        // If we reached gate tile, finish
        if (idx >= pathRef.size()) {
            pathCache.erase(key);
            pathIndex.erase(key);
            g.frightened_mode = 0;           // show full body
            g.releaseState = R_EXITING_GATE; // resume normal exit
        }
        return;
    }

    // Normalize and move
    float step = speed;
    float vx = dx / len * step;
    float vy = dy / len * step;

    // Before moving, ensure we are drawing full body (per your request to see the body)
    // NOTE: we set frightened_mode to 0 while the body moves back so the normal draw function shows the body.
    g.frightened_mode = 0;

    g.position.x += vx;
    g.position.y += vy;
}

// Flee from Pac-Man: Move in the opposite direction of Pac-Man
void fleeFromPacman(Ghost& g, Pacman& p, Map& map, int tileSize, float speed) {
    // Get Pac-Man's grid position
    int px = (int)((p.x + tileSize / 2) / tileSize);
    int py = (int)((p.y + tileSize / 2) / tileSize);

    // Get ghost's grid position
    int gx = (int)((g.position.x + tileSize / 2) / tileSize);
    int gy = (int)((g.position.y + tileSize / 2) / tileSize);

    // Calculate vector from ghost to Pac-Man
    int dx = px - gx;
    int dy = py - gy;

    // Flee: Move in the opposite direction (away from Pac-Man)
    int fleeX = gx - dx;  // Opposite X
    int fleeY = gy - dy;  // Opposite Y

    // Clamp to map bounds
    if (fleeX < 0) fleeX = 0;
    if (fleeX >= map.cols) fleeX = map.cols - 1;
    if (fleeY < 0) fleeY = 0;
    if (fleeY >= map.rows) fleeY = map.rows - 1;

    // Navigate to the flee target at reduced speed (70% of normal)
    navigateToTile(g, map, tileSize, fleeX, fleeY, speed * 0.7f);

}


// -------------------- Convenience wrappers --------------------

// chase wrapper: compute pacman tile then navigate to it with chase speed
void chasePacmanToTarget(Ghost& g, Pacman& p, Map& map, int tileSize) {
    // Optionally allow non-red ghosts to call this; caller may decide
    int px = (int)((p.x + tileSize / 2) / tileSize);
    int py = (int)((p.y + tileSize / 2) / tileSize);
    navigateToTile(g, map, tileSize, px, py, 1.5f);
}

// scatter wrapper: find sensible top-right (or other provided) and navigate with scatter speed
void scatterToCorner(Ghost& g, Map& map, int tileSize) {
    // pick a top-right non-wall tile (search top rows from right to left)
    int tx = -1, ty = -1;
    for (int row = 0; row < map.rows; ++row) {
        for (int col = map.cols - 1; col >= 0; --col) {
            if (!map.isWall(col, row)) { tx = col; ty = row; break; }
        }
        if (tx != -1) break;
    }
    if (tx == -1) { tx = map.cols - 2; ty = 1; } // fallback
    navigateToTile(g, map, tileSize, tx, ty, 1.5f);
}

// Generic wrapper if you want to directly call with any tile:
void navigateGhostToTile(Ghost& g, Map& map, int tileSize, int targetCol, int targetRow, float speed) {
    navigateToTile(g, map, tileSize, targetCol, targetRow, speed);
}

// Global function to release a ghost (Pink/Blue/Orange) relative to when RED left the gate.
// DOES NOT require modifying the Ghost class. Uses an external ReleaseInfo for per-ghost state.
// framesSinceStart: integer frame counter (incremented in main loop)
void releaseGhost(
    Ghost& g,
    Ghost& red,
    Map& map,
    int tileSize,
    int framesSinceStart,
    ReleaseInfo& info   // external per-ghost state you must supply
) {
    const int GATE_EXIT_Y = 7;       // gate row as in your code
    const float exitSpeed = 2.0f;    // speed when exiting gate

    // delays measured from the moment RED leaves the gate (frames @ 60 FPS)
    const int DELAY_PINK_FRAMES = 8 * 60;   // Pink: 8s after Red left
    const int DELAY_ORANGE_FRAMES = 13 * 60;   // Orange: 13s after Red left (5s after Pink)
    const int DELAY_BLUE_FRAMES = 16 * 60;   // Blue: 16s after Red left (8s after Pink)

    // static stores when RED first left the cage (set once)
    static int redLeftFrame = -1;
    // detect red leaving gate (first time)
    if (redLeftFrame == -1) {
        if ((int)(red.position.y / tileSize) < GATE_EXIT_Y) {
            redLeftFrame = framesSinceStart;
        }
    }

    // If red hasn't left yet, keep others in cage
    if (redLeftFrame == -1) return;

    // determine target absolute frame for this ghost based on its id
    int targetAbsFrame = INT_MAX;
    if (g.id == 1) targetAbsFrame = redLeftFrame + DELAY_PINK_FRAMES;     // pink
    else if (g.id == 3) targetAbsFrame = redLeftFrame + DELAY_ORANGE_FRAMES; // orange
    else if (g.id == 2) targetAbsFrame = redLeftFrame + DELAY_BLUE_FRAMES;   // blue
    else return; // non-managed ghost (e.g., red), just return

    // State machine using external ReleaseInfo
    switch (info.state) {
    case R_IN_CAGE:
        // Wait until global frames reach the target
        if (framesSinceStart >= targetAbsFrame) {
            info.state = R_EXITING_GATE;
            info.timer = 0;
        }
        break;

    case R_EXITING_GATE:
        // Move ghost upwards out of the gate
        g.position.y -= exitSpeed;

        // When ghost's grid row <= GATE_EXIT_Y, mark ACTIVE and snap to center
        if ((int)(g.position.y / tileSize) <= GATE_EXIT_Y) {
            info.state = R_ACTIVE;

            // Snap to tile center to avoid jitter
            g.position.x = ((int)(g.position.x / tileSize) * tileSize) + tileSize / 2.0f;
            g.position.y = ((int)(g.position.y / tileSize) * tileSize) + tileSize / 2.0f;

            info.timer = 0;
            info.justEnteredScatter = true; // optional: mimic previous behavior
        }
        break;

    case R_ACTIVE:
        // nothing here; ghost's normal update (chase/scatter) should run in its update() only when ACTIVE
        break;
    }
}


void frightened(
    Pacman& pac,
    RedGhost& red,
    PinkGhost& pink,
    OrangeGhost& orange,
    BlueGhost& blue,
    int& frightenedTimer,   // global timer for this mode
    int tileSize,
    ReleaseInfo& redInfo,
    ReleaseInfo& pinkInfo,
    ReleaseInfo& orangeInfo,
    ReleaseInfo& blueInfo,
    Map& map,
    int& framesSinceStart
)
{
    vector<Ghost*> ghosts = { &red, &pink, &orange, &blue };

    // Increment frightened timer
    frightenedTimer++;

    bool flashing = (frightenedTimer >= FRIGHTENED_FLASH_START) && (frightenedTimer % 30 < 15);


    // On first frame of frightened mode, reverse all ghost directions
    if (frightenedTimer == 1) {
        for (Ghost* g : ghosts) {
            if (g->frightened_mode != 2) {
                // Reverse by flipping direction (if used, but BFS overrides)
                g->direction = (g->direction + 2) % 4;
            }
        }
    }
    for (Ghost* g : ghosts) {
        if (g->frightened_mode != 2) {
            g->frightened_mode = 1;
        }
    }

    // Set all ghosts to frightened mode
    for (Ghost* g : ghosts)
    {
        if (g->frightened_mode != 2) // not eaten
        {
            g->frightened_mode = 1; // royal blue
            // Draw function will handle flashing color
        }
    }

    // Handle collisions: Pacman is chaser
    for (Ghost* g : ghosts)
    {
        float dx = fabs(pac.x - g->position.x);  //CH1
        float dy = fabs(pac.y - g->position.y);

        if (dx < 20 && dy < 20) // collision
        {
            // Send ghost to cage
            g->frightened_mode = 2; // eyes
            g->position.x = (float)g->cageX * tileSize;
            g->position.y = (float)g->cageY * tileSize;

            // score logic
            pac.score += 200;

            // Reset release state to exit immediately
            switch (g->id) {
            case 0: redInfo.state = R_ACTIVE; redInfo.timer = 0; break;
            case 1: pinkInfo.state = R_EXITING_GATE; pinkInfo.timer = 0; break;
            case 2: blueInfo.state = R_EXITING_GATE; blueInfo.timer = 0; break;
            case 3: orangeInfo.state = R_EXITING_GATE; orangeInfo.timer = 0; break;
            }

            // Optional: add score for eating ghost
            //pac.score += 200;
        }
    }

    // End frightened mode after 7 seconds
    if (frightenedTimer >= FRIGHTENED_TOTAL_FRAMES) {
        frightenedTimer = 0;
        for (Ghost* g : ghosts) {
            if (g->frightened_mode != 2) {
                g->frightened_mode = 0;
            }
        }
    }
}

void checkPacmanGhostCollision(
    Pacman& pac,
    RedGhost& red,
    PinkGhost& pink,
    OrangeGhost& orange,
    BlueGhost& blue,
    int tileSize,
    ReleaseInfo& redInfo,
    ReleaseInfo& blueInfo,
    ReleaseInfo& pinkInfo,
    ReleaseInfo& orangeInfo,
    Map& map,
    int& framesSinceStart
)
{
    // Local static to reset releaseGhost timing after a death
    static int redLeftFrame_global = -1;

    std::vector<Ghost*> ghosts = { &red, &pink, &orange, &blue };

    for (Ghost* g : ghosts)
    {
        float pacCenterX = pac.x + tileSize / 2.0f;
        float pacCenterY = pac.y + tileSize / 2.0f;
        float ghostCenterX = g->position.x + tileSize / 2.0f;
        float ghostCenterY = g->position.y + tileSize / 2.0f;

        float dx = pacCenterX - ghostCenterX;
        float dy = pacCenterY - ghostCenterY;

        float distSq = dx * dx + dy * dy;

        float pacRadius = pac.radius;
        float ghostRadius = tileSize * 0.40f;
        float collisionDist = pacRadius + ghostRadius;
        float collisionDistSq = collisionDist * collisionDist;

        if (distSq < collisionDistSq)
        {
            // ------------------ IF GHOST IS FRIGHTENED ------------------
            if (g->frightened_mode == 1) // ghost is edible
            {
                g->frightened_mode = 2; // eyes mode
                g->position.x = (float)g->cageX * tileSize;
                g->position.y = (float)g->cageY * tileSize;
                //THESE 2 LINES SEND GHOST BACK TO GATE
                //IN FRIGHTENED MODE AND STOP COLLISION
                // IN FRIGHETENED MODE FROM ACTING LIKE IN SCATTERED M 
                switch (g->id) {
                case 0: redInfo.state = R_EXITING_GATE; redInfo.timer = 0; break;
                case 1: pinkInfo.state = R_EXITING_GATE; pinkInfo.timer = 0; break;
                case 2: blueInfo.state = R_EXITING_GATE; blueInfo.timer = 0; break;
                case 3: orangeInfo.state = R_EXITING_GATE; orangeInfo.timer = 0; break;
                }

                continue; // skip Pac-Man death
            }

            // ------------------ ELSE PACMAN DIES ------------------
            pac.dying = true;
            pac.death_timer = 0;
            pac.lives--;

            // Reset Pac-Man position
            pac.x = pac.startXPos;
            pac.y = pac.startYPos;
            pac.direction = Pacman::RIGHT;
            pac.alive = true;

            // Reset all ghosts back to cage
            const int RED_CAGE_COL = 10, RED_CAGE_ROW = 8;
            const int PINK_CAGE_COL = 9, PINK_CAGE_ROW = 9;
            const int ORANGE_CAGE_COL = 10, ORANGE_CAGE_ROW = 9;  // Changed from 10 to 11
            const int BLUE_CAGE_COL = 11, BLUE_CAGE_ROW = 9;

            red.position = { (float)red.cageX * tileSize, (float)red.cageY * tileSize };
            pink.position = { (float)pink.cageX * tileSize, (float)pink.cageY * tileSize };
            orange.position = { (float)orange.cageX * tileSize, (float)orange.cageY * tileSize };
            blue.position = { (float)blue.cageX * tileSize, (float)blue.cageY * tileSize };

            // Clear frightened state
            red.frightened_mode = 0;
            pink.frightened_mode = 0;
            orange.frightened_mode = 0;
            blue.frightened_mode = 0;

            // Reset release states
            redInfo.state = R_ACTIVE; redInfo.timer = 0; redInfo.justEnteredScatter = false;
            pinkInfo.state = R_IN_CAGE; pinkInfo.timer = 0; pinkInfo.justEnteredScatter = false;
            orangeInfo.state = R_IN_CAGE; orangeInfo.timer = 0; orangeInfo.justEnteredScatter = false;
            blueInfo.state = R_IN_CAGE; blueInfo.timer = 0; blueInfo.justEnteredScatter = false;

            framesSinceStart = 0;
            redLeftFrame_global = -1;

            // Restart release cycle
            releaseGhost(pink, red, map, tileSize, framesSinceStart, pinkInfo);
            releaseGhost(orange, red, map, tileSize, framesSinceStart, orangeInfo);
            releaseGhost(blue, red, map, tileSize, framesSinceStart, blueInfo);

            return; // very important: avoid multi-collision same frame
        }
    }
}




