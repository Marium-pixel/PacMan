
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

    bool flashing = false;
    if (frightenedTimer >= FRIGHTENED_FLASH_START) {
        // last 2 seconds: flash every 15 frames
        flashing = (frightenedTimer % 30 < 15);
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
        float dx = fabs(pac.x - g->position.x);
        float dy = fabs(pac.y - g->position.y);

        if (dx < 20 && dy < 20) // collision
        {
            // Send ghost to cage
            g->frightened_mode = 2; // eyes
            g->position.x = g->cageX * tileSize;
            g->position.y = g->cageY * tileSize;

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
    if (frightenedTimer >= FRIGHTENED_TOTAL_FRAMES)
    {
        frightenedTimer = 0; // reset timer

        for (Ghost* g : ghosts) {
            if (g->frightened_mode != 2) {
                g->frightened_mode = 0; // back to normal chase/scatter
            }
        }
    }
}
