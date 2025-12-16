#include "TetrisGame.h"
#include "BlockTemplate.h"
#include "SoundManager.h"
#include "Board.h"
#include <iostream>

#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <cstdio>

static const std::string HIGH_SCORE_FILE = "highscores.txt";

TetrisGame::TetrisGame() {
    std::random_device rd;
    rng.seed(rd());
    loadHighScores();
}

void TetrisGame::loadHighScores() {
    state.highScores.clear();
    std::ifstream file(HIGH_SCORE_FILE);
    int scoreVal;

    if (file.is_open()) {
        while (file >> scoreVal) {
            state.highScores.push_back(scoreVal);
        }
        file.close();

        // Keep scores sorted in descending order.
        std::sort(state.highScores.begin(),
                  state.highScores.end(),
                  std::greater<int>());
    }
}

void TetrisGame::drawStartScreen() {
    std::string screen;
    screen.reserve(512);

    // Clear screen and move cursor to top\-left.
    screen += "\033[2J\033[1;1H";

    int totalWidth = (BOARD_WIDTH * 2) + 13; // Match in\-game layout.

    // Top border.
    screen += "╔";
    for (int i = 0; i < totalWidth; ++i) screen += "═";
    screen += "╗\n";

    // Spacer row.
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Title row.
    const std::string title = "TETRIS GAME";
    int titlePadding        = totalWidth - static_cast<int>(title.length());
    int titleLeft           = titlePadding / 2;
    int titleRight          = titlePadding - titleLeft;

    screen += "║";
    screen.append(titleLeft, ' ');
    screen += title;
    screen.append(titleRight, ' ');
    screen += "║\n";

    // Spacer row.
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Prompt row.
    const std::string prompt = "Press any key to start...";
    int promptPadding        = totalWidth - static_cast<int>(prompt.length());
    int promptLeft           = promptPadding / 2;
    int promptRight          = promptPadding - promptLeft;

    screen += "║";
    screen.append(promptLeft, ' ');
    screen += prompt;
    screen.append(promptRight, ' ');
    screen += "║\n";

    // Spacer row.
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Bottom border.
    screen += "╚";
    for (int i = 0; i < totalWidth; ++i) screen += "═";
    screen += "╝\n";

    std::cout << screen;
    std::cout.flush();
}

char TetrisGame::waitForKeyPress() {
    enableRawMode();

    char key = 0;
    // Poll non\-blocking input until one key is received.
    while ((key = getInput()) == 0) {
        usleep(50000);
    }

    flushInput();
    return key;
}

int TetrisGame::saveAndGetRank() {
    // Load any existing scores.
    std::vector<int> scores;
    std::ifstream inFile(HIGH_SCORE_FILE);
    if (inFile.is_open()) {
        int score;
        while (inFile >> score) {
            scores.push_back(score);
        }
        inFile.close();
    }

    // Add current run score.
    scores.push_back(state.score);

    // Sort highest to lowest.
    std::sort(scores.begin(), scores.end(), std::greater<int>());

    // Keep only top 10.
    if (scores.size() > 10) {
        scores.resize(10);
    }

    // Save back to file.
    std::ofstream outFile(HIGH_SCORE_FILE);
    if (outFile.is_open()) {
        for (int score : scores) {
            outFile << score << '\n';
        }
        outFile.close();
    }

    // Compute 1\-based rank of current score.
    int rank = 1;
    for (int score : scores) {
        if (score == state.score) break;
        ++rank;
    }

    return rank;
}

void TetrisGame::drawGameOverScreen(int rank) {
    SoundManager::playGameOverSound();

    std::string screen;
    screen.reserve(1024);

    // Clear screen and reset cursor.
    screen += "\033[2J\033[1;1H";

    int totalWidth = (BOARD_WIDTH * 2) + 13;

    // Top border.
    screen += "╔";
    for (int i = 0; i < totalWidth; ++i) screen += "═";
    screen += "╗\n";

    // Spacer.
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // "GAME OVER" title.
    const std::string title = "GAME OVER";
    int titlePadding        = totalWidth - static_cast<int>(title.length());
    int titleLeft           = titlePadding / 2;
    int titleRight          = titlePadding - titleLeft;

    screen += "║";
    screen.append(titleLeft, ' ');
    screen += title;
    screen.append(titleRight, ' ');
    screen += "║\n";

    // Spacer.
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Final score row (label left, value right).
    std::string scoreLabel = "Final Score:";
    std::string scoreNum   = std::to_string(state.score);
    int scoreSpacing       =
        totalWidth - static_cast<int>(scoreLabel.length()) -
        static_cast<int>(scoreNum.length());

    screen += "║ ";
    screen += scoreLabel;
    screen.append(scoreSpacing - 2, ' ');
    screen += scoreNum;
    screen += " ║\n";

    // Level row.
    std::string levelLabel = "Level:";
    std::string levelNum   = std::to_string(state.level);
    int levelSpacing       =
        totalWidth - static_cast<int>(levelLabel.length()) -
        static_cast<int>(levelNum.length());

    screen += "║ ";
    screen += levelLabel;
    screen.append(levelSpacing - 2, ' ');
    screen += levelNum;
    screen += " ║\n";

    // Lines row.
    std::string linesLabel = "Lines Cleared:";
    std::string linesNum   = std::to_string(state.linesCleared);
    int linesSpacing       =
        totalWidth - static_cast<int>(linesLabel.length()) -
        static_cast<int>(linesNum.length());

    screen += "║ ";
    screen += linesLabel;
    screen.append(linesSpacing - 2, ' ');
    screen += linesNum;
    screen += " ║\n";

    // Spacer.
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Rank row with ordinal suffix.
    char rankBuf[64];
    const char* suffix = "th";
    if (rank == 1)      suffix = "st";
    else if (rank == 2) suffix = "nd";
    else if (rank == 3) suffix = "rd";

    std::snprintf(rankBuf, sizeof(rankBuf), "Your Rank: %d%s", rank, suffix);
    std::string rankStr(rankBuf);
    int rankPadding = totalWidth - static_cast<int>(rankStr.length());
    int rankLeft    = rankPadding / 2;
    int rankRight   = rankPadding - rankLeft;

    screen += "║";
    screen.append(rankLeft, ' ');
    screen += rankStr;
    screen.append(rankRight, ' ');
    screen += "║\n";

    // Spacer.
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // High score list (top N).
    for (size_t i = 0; i < state.highScores.size(); ++i) {
        std::string suff = "th";
        if (i == 0)      suff = "st";
        else if (i == 1) suff = "nd";
        else if (i == 2) suff = "rd";

        std::string rankLabel  = std::to_string(i + 1) + suff;
        std::string scoreStr   = std::to_string(state.highScores[i]);
        bool isNew             =
            (state.score > 0 && state.score == state.highScores[i]);

        if (isNew) {
            scoreStr += " NEW!";
        }

        int contentWidth = static_cast<int>(rankLabel.length() +
                                            scoreStr.length());
        int spacing      = totalWidth - contentWidth - 2;
        if (spacing < 1) spacing = 1;

        screen += "║ ";
        screen += rankLabel;
        screen.append(spacing, ' ');
        screen += scoreStr;
        screen += " ║\n";
    }

    // Spacer.
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Restart / quit prompt.
    const std::string prompt = "Press R to Restart or Q to Quit";
    int promptPadding        = totalWidth - static_cast<int>(prompt.length());
    int promptLeft           = promptPadding / 2;
    int promptRight          = promptPadding - promptLeft;

    screen += "║";
    screen.append(promptLeft, ' ');
    screen += prompt;
    screen.append(promptRight, ' ');
    screen += "║\n";

    // Spacer.
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Bottom border.
    screen += "╚";
    for (int i = 0; i < totalWidth; ++i) screen += "═";
    screen += "╝\n";

    std::cout << screen;
    std::cout.flush();
}

void TetrisGame::resetGame() {
    // Reset core state to a fresh run.
    state.running      = true;
    state.paused       = false;
    state.quitByUser   = false;
    state.score        = 0;
    state.level        = 1;
    state.linesCleared = 0;

    board.init();
    dropCounter = 0;

    // Pre\-roll next piece and spawn the first current piece.
    std::uniform_int_distribution<int> dist(0,
        BlockTemplate::NUM_BLOCK_TYPES - 1);
    nextPieceType = dist(rng);
    spawnNewPiece();
}

// \=== Terminal raw mode handling ===

void TetrisGame::enableRawMode() {
    // Save current settings then disable canonical mode & echo.
    tcgetattr(STDIN_FILENO, &origTermios);

    termios raw = origTermios;
    raw.c_lflag &= ~(ICANON | ECHO); // Raw input, no echo
    raw.c_cc[VMIN]  = 0;             // Non\-blocking read
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // Make STDIN non\-blocking at the file descriptor level as well.
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void TetrisGame::disableRawMode() {
    // Restore original terminal settings.
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios);
}

char TetrisGame::getInput() const {
    char ch = 0;
    ssize_t result = read(STDIN_FILENO, &ch, 1);
    if (result <= 0) return 0;

    if (ch == 27) { // ESC sequence (likely arrow keys)
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) <= 0) return 27;
        if (read(STDIN_FILENO, &seq[1], 1) <= 0) return 27;

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return 'w'; // Up    -> rotate
                case 'B': return 's'; // Down  -> soft drop
                case 'C': return 'd'; // Right -> move right
                case 'D': return 'a'; // Left  -> move left
            }
        }
        return 27;
    }

    return ch;
}

void TetrisGame::flushInput() const {
    // Discard any pending input in the terminal buffer.
    tcflush(STDIN_FILENO, TCIFLUSH);
}

// \=== Game logic ===

void TetrisGame::animateGameOver() {
    // Turn all locked blocks into '#' from bottom to top for a wave effect.
    for (int y = BOARD_HEIGHT - 1; y >= 0; --y) {
        bool hasBlock = false;
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            if (board.grid[y][x] != ' ') {
                hasBlock = true;
                board.grid[y][x] = '#';

                std::string preview[4];
                getNextPiecePreview(preview);
                board.draw(state, preview);

                usleep(ANIM_DELAY_US);
            }
        }

        if (!hasBlock) {
            // Skip empty rows more quickly.
            continue;
        }
    }

    flushInput();
    usleep(500000); // Short pause to let user see final frame.
    flushInput();
}

bool TetrisGame::isInsidePlayfield(int x, int y) const {
    return x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT;
}

Piece TetrisGame::calculateGhostPiece() const {
    // Copy current piece and drop it down until collision.
    Piece ghost = currentPiece;

    bool canMoveDown = true;
    while (canMoveDown) {
        canMoveDown = false;

        // Test collision one row below the current ghost position.
        for (int row = 0; row < BlockTemplate::BLOCK_SIZE; ++row) {
            for (int col = 0; col < BlockTemplate::BLOCK_SIZE; ++col) {
                char cell = BlockTemplate::getCell(
                    ghost.type, ghost.rotation, row, col
                );
                if (cell == ' ') continue;

                int xt = ghost.pos.x + col;
                int yt = ghost.pos.y + row + 1; // test next row down

                // If out of bottom bound, stop.
                if (yt >= BOARD_HEIGHT) {
                    goto done_checking;
                }

                if (yt >= 0) {
                    // Only collide with real blocks, not ghost cells '.'.
                    char gridCell = board.grid[yt][xt];
                    if (gridCell != ' ' && gridCell != '.') {
                        goto done_checking;
                    }
                }
            }
        }

        canMoveDown = true;
    done_checking:
        if (canMoveDown) {
            ++ghost.pos.y;
        }
    }

    return ghost;
}

bool TetrisGame::canSpawn(const Piece& piece) const {
    // Check that the spawn position does not overlap locked cells
    // and stays inside horizontal bounds.
    for (int row = 0; row < BlockTemplate::BLOCK_SIZE; ++row) {
        for (int col = 0; col < BlockTemplate::BLOCK_SIZE; ++col) {
            char cell = BlockTemplate::getCell(
                piece.type, piece.rotation, row, col
            );
            if (cell == ' ') continue;

            int xt = piece.pos.x + col;
            int yt = piece.pos.y + row;

            if (xt < 0 || xt >= BOARD_WIDTH)   return false;
            if (yt >= BOARD_HEIGHT)            return false;

            if (yt >= 0) {
                char gridCell = board.grid[yt][xt];
                if (gridCell != ' ' && gridCell != '.') {
                    return false;
                }
            }
        }
    }
    return true;
}

bool TetrisGame::canMove(int dx, int dy, int newRotation) const {
    // Test moving the current piece by (dx, dy) and rotation newRotation
    // against board bounds and collisions.
    for (int row = 0; row < BlockTemplate::BLOCK_SIZE; ++row) {
        for (int col = 0; col < BlockTemplate::BLOCK_SIZE; ++col) {
            char cell = BlockTemplate::getCell(
                currentPiece.type, newRotation, row, col
            );
            if (cell == ' ') continue;

            int xt = currentPiece.pos.x + col + dx;
            int yt = currentPiece.pos.y + row + dy;

            if (xt < 0 || xt >= BOARD_WIDTH)   return false;
            if (yt >= BOARD_HEIGHT)            return false;

            if (yt >= 0) {
                char gridCell = board.grid[yt][xt];
                if (gridCell != ' ' && gridCell != '.') {
                    return false;
                }
            }
        }
    }
    return true;
}

void TetrisGame::placePiece(const Piece& piece, bool place) {
    // Write or erase the piece in the board grid.
    for (int row = 0; row < BlockTemplate::BLOCK_SIZE; ++row) {
        for (int col = 0; col < BlockTemplate::BLOCK_SIZE; ++col) {
            char cell = BlockTemplate::getCell(
                piece.type, piece.rotation, row, col
            );
            if (cell == ' ') continue;

            int xt = piece.pos.x + col;
            int yt = piece.pos.y + row;

            if (!isInsidePlayfield(xt, yt)) continue;

            board.grid[yt][xt] = place ? cell : ' ';
        }
    }
}

void TetrisGame::clearAllGhostDots() {
    // Only clear cells that were previously marked as ghost '.'
    // for better performance than scanning the entire board.
    for (const Position& pos : lastGhostPositions) {
        if (isInsidePlayfield(pos.x, pos.y) &&
            board.grid[pos.y][pos.x] == '.') {
            board.grid[pos.y][pos.x] = ' ';
        }
    }
    lastGhostPositions.clear();
}

void TetrisGame::placeGhostPiece(const Piece& ghostPiece) {
    // Draw ghost outline as '.' into empty cells only.
    for (int row = 0; row < BlockTemplate::BLOCK_SIZE; ++row) {
        for (int col = 0; col < BlockTemplate::BLOCK_SIZE; ++col) {
            char cell = BlockTemplate::getCell(
                ghostPiece.type, ghostPiece.rotation, row, col
            );
            if (cell == ' ') continue;

            int xt = ghostPiece.pos.x + col;
            int yt = ghostPiece.pos.y + row;

            if (!isInsidePlayfield(xt, yt)) continue;

            if (board.grid[yt][xt] == ' ') {
                board.grid[yt][xt] = '.';
                lastGhostPositions.emplace_back(xt, yt);
            }
        }
    }
}

void TetrisGame::placePieceSafe(const Piece& piece) {
    // Like placePiece(true) but does not overwrite non\-empty cells.
    for (int row = 0; row < BlockTemplate::BLOCK_SIZE; ++row) {
        for (int col = 0; col < BlockTemplate::BLOCK_SIZE; ++col) {
            char cell = BlockTemplate::getCell(
                piece.type, piece.rotation, row, col
            );
            if (cell == ' ') continue;

            int xt = piece.pos.x + col;
            int yt = piece.pos.y + row;

            if (!isInsidePlayfield(xt, yt)) continue;

            if (board.grid[yt][xt] == ' ') {
                board.grid[yt][xt] = cell;
            }
        }
    }
}

void TetrisGame::spawnNewPiece() {
    std::uniform_int_distribution<int> dist(0,
        BlockTemplate::NUM_BLOCK_TYPES - 1);

    Piece spawn;
    spawn.type      = nextPieceType;
    spawn.rotation  = 0;
    int spawnX      = (BOARD_WIDTH / 2) - (BlockTemplate::BLOCK_SIZE / 2);
    spawn.pos       = Position(spawnX, -1);

    currentPiece = spawn;

    if (!canSpawn(spawn)) {
        // If the new piece cannot spawn, the game is over.
        state.running = false;
        return;
    }

    nextPieceType = dist(rng);
}

bool TetrisGame::lockPieceAndCheck(bool muteLockSound) {
    // Permanently place current piece and clear lines if needed.
    placePiece(currentPiece, true);

    int lines = board.clearLines();
    if (lines > 0) {
        // Different sound for 4\-line clears.
        if (lines == 4) {
            SoundManager::play4LinesClearSound();
        } else {
            SoundManager::playLineClearSound();
        }

        state.linesCleared += lines;

        // Simple scoring: base scores multiplied by current level.
        const int scores[] = {0, 100, 300, 500, 800};
        state.score += scores[lines] * state.level;

        int oldLevel = state.level;
        state.level = 1 + (state.linesCleared / 10); // +1 level per 10 lines

        if (state.level > oldLevel) {
            SoundManager::playLevelUpSound();
        }

        updateDifficulty();
    } else if (!muteLockSound) {
        SoundManager::playLockPieceSound();
    }

    spawnNewPiece();
    return state.running;
}

void TetrisGame::softDrop() {
    // Try to move piece down by one row, or lock if colliding.
    if (canMove(0, 1, currentPiece.rotation)) {
        ++currentPiece.pos.y;
    } else {
        if (currentPiece.pos.y < 0) {
            // Locked above visible board => instant game over.
            state.running = false;
            return;
        }
        state.running = lockPieceAndCheck(true);
        dropCounter   = 0;
    }
}

void TetrisGame::hardDrop() {
    // Move piece down until it hits something, then lock.
    while (canMove(0, 1, currentPiece.rotation)) {
        ++currentPiece.pos.y;
    }

    if (currentPiece.pos.y < 0) {
        state.running = false;
        return;
    }

    state.running = lockPieceAndCheck(true);
    dropCounter   = 0;
}

void TetrisGame::handleInput() {
    char c = getInput();
    if (c == 0) return;

    // Toggle pause.
    if (c == 'p') {
        state.paused = !state.paused;
        flushInput();
        if (state.paused) {
            drawPauseScreen();
        }
        return;
    }

    // Toggle ghost piece, allowed even when paused.
    if (c == 'g') {
        state.ghostEnabled = !state.ghostEnabled;
        return;
    }

    if (state.paused) {
        // While paused, only 'q' is handled to quit.
        if (c == 'q') {
            state.running   = false;
            state.quitByUser = true;
            SoundManager::stopBackgroundSound();
        }
        return;
    }

    // Gameplay key handling.
    switch (c) {
        case 'a': // move left
            if (canMove(-1, 0, currentPiece.rotation)) {
                --currentPiece.pos.x;
            }
            break;
        case 'd': // move right
            if (canMove(1, 0, currentPiece.rotation)) {
                ++currentPiece.pos.x;
            }
            break;
        case 's': // soft drop
            SoundManager::playSoftDropSound();
            softDrop();
            break;
        case ' ': // hard drop
            SoundManager::playHardDropSound();
            hardDrop();
            flushInput();
            break;
        case 'w': { // rotate clockwise with simple wall kicks
            int newRot = (currentPiece.rotation + 1) % 4;
            int kicks[] = {0, -1, 1, -2, 2, -3, 3};
            for (int dx : kicks) {
                if (canMove(dx, 0, newRot)) {
                    currentPiece.pos.x += dx;
                    currentPiece.rotation = newRot;
                    break;
                }
            }
            break;
        }
        case 'q': // quit
            state.running    = false;
            state.quitByUser = true;
            SoundManager::stopBackgroundSound();
            break;
        default:
            break;
    }
}

void TetrisGame::handleGravity() {
    if (!state.running || state.paused) return;

    ++dropCounter;
    if (dropCounter < DROP_INTERVAL_TICKS) {
        return;
    }

    dropCounter = 0;

    if (canMove(0, 1, currentPiece.rotation)) {
        ++currentPiece.pos.y;
    } else {
        if (currentPiece.pos.y < 0) {
            state.running = false;
            return;
        }
        state.running = lockPieceAndCheck();
    }
}

void TetrisGame::getNextPiecePreview(std::string lines[4]) {
    // Reuse cached preview if the next type hasn't changed.
    if (cachedNextPieceType == nextPieceType) {
        for (int i = 0; i < 4; ++i) {
            lines[i] = cachedNextPiecePreview[i];
        }
        return;
    }

    // Render 4 rows of the 4x4 template as colorized "██" + spaces.
    for (int row = 0; row < 4; ++row) {
        cachedNextPiecePreview[row].clear();
        cachedNextPiecePreview[row].reserve(64);

        for (int col = 0; col < 4; ++col) {
            char cell = BlockTemplate::getCell(nextPieceType, 0, row, col);
            if (cell != ' ') {
                cachedNextPiecePreview[row] +=
                    PIECE_COLORS[nextPieceType];
                cachedNextPiecePreview[row].append("██");
                cachedNextPiecePreview[row] += COLOR_RESET;
            } else {
                cachedNextPiecePreview[row].append("  ");
            }
        }

        lines[row] = cachedNextPiecePreview[row];
    }

    cachedNextPieceType = nextPieceType;
}

long TetrisGame::computeDropSpeedUs(int level) const {
    // Piece fall speed profile per level range.
    if (level <= 3) {          // Slow early levels
        return 500000;         // 0.50s per tick group
    } else if (level <= 6) {   // Medium
        return 300000;         // 0.30s
    } else if (level <= 9) {   // Fast
        return 150000;         // 0.15s
    } else {                   // Very fast for 10+
        return 80000;          // 0.08s
    }
}

void TetrisGame::updateDifficulty() {
    dropSpeedUs = computeDropSpeedUs(state.level);
}

void TetrisGame::drawPauseScreen() const {
    std::string screen;
    screen.reserve(1024);

    screen += "\033[2J\033[1;1H";

    int totalWidth = (BOARD_WIDTH * 2) + 13;

    // Top border.
    screen += "╔";
    for (int i = 0; i < totalWidth; ++i) screen += "═";
    screen += "╗\n";

    // Some spacer rows.
    for (int i = 0; i < 3; ++i) {
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";
    }

    // "GAME PAUSED" title.
    const std::string title = "GAME PAUSED";
    int titlePadding        = totalWidth - static_cast<int>(title.length());
    int titleLeft           = titlePadding / 2;
    int titleRight          = titlePadding - titleLeft;

    screen += "║";
    screen.append(titleLeft, ' ');
    screen += title;
    screen.append(titleRight, ' ');
    screen += "║\n";

    // Spacer.
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Score row.
    char scoreBuf[64];
    std::snprintf(scoreBuf, sizeof(scoreBuf), "Score: %d", state.score);
    std::string scoreStr(scoreBuf);
    int scorePadding = totalWidth - static_cast<int>(scoreStr.length());
    int scoreLeft    = scorePadding / 2;
    int scoreRight   = scorePadding - scoreLeft;

    screen += "║";
    screen.append(scoreLeft, ' ');
    screen += scoreStr;
    screen.append(scoreRight, ' ');
    screen += "║\n";

    // Level row.
    char levelBuf[64];
    std::snprintf(levelBuf, sizeof(levelBuf), "Level: %d", state.level);
    std::string levelStr(levelBuf);
    int levelPadding = totalWidth - static_cast<int>(levelStr.length());
    int levelLeft    = levelPadding / 2;
    int levelRight   = levelPadding - levelLeft;

    screen += "║";
    screen.append(levelLeft, ' ');
    screen += levelStr;
    screen.append(levelRight, ' ');
    screen += "║\n";

    // Lines row.
    char linesBuf[64];
    std::snprintf(linesBuf, sizeof(linesBuf), "Lines: %d",
                  state.linesCleared);
    std::string linesStr(linesBuf);
    int linesPadding = totalWidth - static_cast<int>(linesStr.length());
    int linesLeft    = linesPadding / 2;
    int linesRight   = linesPadding - linesLeft;

    screen += "║";
    screen.append(linesLeft, ' ');
    screen += linesStr;
    screen.append(linesRight, ' ');
    screen += "║\n";

    // Spacer.
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Options: Resume and Quit.
    const std::string resumeOption = "P - Resume";
    int resumePadding = totalWidth - static_cast<int>(resumeOption.length());
    int resumeLeft    = resumePadding / 2;
    int resumeRight   = resumePadding - resumeLeft;

    screen += "║";
    screen.append(resumeLeft, ' ');
    screen += resumeOption;
    screen.append(resumeRight, ' ');
    screen += "║\n";

    const std::string quitOption = "Q - Quit";
    int quitPadding = totalWidth - static_cast<int>(quitOption.length());
    int quitLeft    = quitPadding / 2;
    int quitRight   = quitPadding - quitLeft;

    screen += "║";
    screen.append(quitLeft, ' ');
    screen += quitOption;
    screen.append(quitRight, ' ');
    screen += "║\n";

    // Bottom spacers.
    for (int i = 0; i < 3; ++i) {
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";
    }

    // Bottom border.
    screen += "╚";
    for (int i = 0; i < totalWidth; ++i) screen += "═";
    screen += "╝\n";

    std::cout << screen;
    std::cout.flush();
}

// \=== Main game loop ===

void TetrisGame::run() {
    BlockTemplate::initializeTemplates();

    bool shouldRestart = true;

    while (shouldRestart) {
        board.init();

        std::uniform_int_distribution<int> dist(
            0, BlockTemplate::NUM_BLOCK_TYPES - 1
        );
        nextPieceType = dist(rng);

        drawStartScreen();
        waitForKeyPress();

        // Restart background music cleanly.
        SoundManager::stopBackgroundSound();
        usleep(100000);
        SoundManager::playBackgroundSound();

        updateDifficulty();
        spawnNewPiece();

        // Core per\-frame game loop.
        while (state.running) {
            handleInput();

            if (state.paused) {
                usleep(100000);
                continue;
            }

            if (!state.running) break;

            handleGravity();

            // Update ghost piece.
            clearAllGhostDots();
            if (state.ghostEnabled) {
                Piece ghost = calculateGhostPiece();
                if (ghost.pos.y != currentPiece.pos.y) {
                    placeGhostPiece(ghost);
                }
            }

            // Draw current piece over board, then remove it again.
            placePiece(currentPiece, true);

            std::string preview[4];
            getNextPiecePreview(preview);
            board.draw(state, preview);

            placePiece(currentPiece, false);

            usleep(dropSpeedUs / DROP_INTERVAL_TICKS);
        }

        if (!state.quitByUser) {
            // Make sure last piece is visible.
            placePieceSafe(currentPiece);

            std::string preview[4];
            getNextPiecePreview(preview);
            board.draw(state, preview);

            flushInput();
            usleep(800000);
            flushInput();

            animateGameOver();
        }

        SoundManager::stopBackgroundSound();

        int rank = saveAndGetRank();
        loadHighScores();
        drawGameOverScreen(rank);

        char choice = waitForKeyPress();
        if (choice == 'r' || choice == 'R') {
            resetGame();
            shouldRestart = true;
        } else {
            shouldRestart = false;
        }

        disableRawMode();
    }
}
