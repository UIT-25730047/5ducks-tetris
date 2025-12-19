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

static const string HIGH_SCORE_FILE = "highscores.txt";

TetrisGame::TetrisGame() {
    random_device rd;
    rng.seed(rd());
    loadHighScores();
}

void TetrisGame::loadHighScores() {
    state.highScores.clear();
    ifstream file(HIGH_SCORE_FILE);
    int scoreVal;

    if (file.is_open()) {
        while (file >> scoreVal) {
            state.highScores.push_back(scoreVal);
        }
        file.close();

        // Keep scores sorted in descending order.
        sort(
            state.highScores.begin(),
            state.highScores.end(),
            greater<int>()
        );
    }
}

void TetrisGame::drawStartScreen() {
    string screen;
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
    const string title = "TETRIS GAME";
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
    const string prompt = "Press any key to start...";
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

    cout << screen;
    cout.flush();
}

char TetrisGame::waitForKeyPress() {
    enableRawMode();

    char key = 0;
    // Liên tục kiểm tra có phím nào được nhấn không
    while ((key = getInput()) == 0) {
        // Ngủ 50ms để giảm tải CPU
        usleep(50000);
    }

    flushInput();
    return key;
}

int TetrisGame::saveAndGetRank() {
    // Đọc file high score
    vector<int> scores;
    ifstream inFile(HIGH_SCORE_FILE);
    if (inFile.is_open()) {
        int score;
        while (inFile >> score) {
            scores.push_back(score);
        }
        inFile.close();
    }

    // Thêm điểm của player hiện tại vào vector
    scores.push_back(state.score);

    // Sắp xếp từ cao đến thấp
    sort(scores.begin(), scores.end(), greater<int>());

    // Chỉ giữ lại top 10
    if (scores.size() > 10) {
        scores.resize(10);
    }

    // Lưu lại vào file
    ofstream outFile(HIGH_SCORE_FILE);
    if (outFile.is_open()) {
        for (int score : scores) {
            outFile << score << '\n';
        }
        outFile.close();
    }

    // Tính thứ hạng của player hiện tại
    int rank = 1;
    for (int score : scores) {
        if (score == state.score) break;
        ++rank;
    }

    return rank;
}

void TetrisGame::drawGameOverScreen(int rank) {
    SoundManager::playGameOverSound();

    string screen;
    screen.reserve(1024);

    // Xóa màn hình
    screen += "\033[2J\033[1;1H";

    int totalWidth = (BOARD_WIDTH * 2) + 13;

    // Top border
    screen += "╔";
    for (int i = 0; i < totalWidth; ++i) screen += "═";
    screen += "╗\n";

    // Spacer
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // "GAME OVER" title
    const string title = "GAME OVER";
    int titlePadding        = totalWidth - static_cast<int>(title.length());
    int titleLeft           = titlePadding / 2;
    int titleRight          = titlePadding - titleLeft;

    screen += "║";
    screen.append(titleLeft, ' ');
    screen += title;
    screen.append(titleRight, ' ');
    screen += "║\n";

    // Spacer
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Final score row (label left, value right)
    string scoreLabel = "Final Score:";
    string scoreNum   = to_string(state.score);
    int scoreSpacing       =
        totalWidth - static_cast<int>(scoreLabel.length()) -
        static_cast<int>(scoreNum.length());

    screen += "║ ";
    screen += scoreLabel;
    screen.append(scoreSpacing - 2, ' ');
    screen += scoreNum;
    screen += " ║\n";

    // Level row
    string levelLabel = "Level:";
    string levelNum   = to_string(state.level);
    int levelSpacing       =
        totalWidth - static_cast<int>(levelLabel.length()) -
        static_cast<int>(levelNum.length());

    screen += "║ ";
    screen += levelLabel;
    screen.append(levelSpacing - 2, ' ');
    screen += levelNum;
    screen += " ║\n";

    // Lines Cleared row
    string linesLabel = "Lines Cleared:";
    string linesNum   = to_string(state.linesCleared);
    int linesSpacing       =
        totalWidth - static_cast<int>(linesLabel.length()) -
        static_cast<int>(linesNum.length());

    screen += "║ ";
    screen += linesLabel;
    screen.append(linesSpacing - 2, ' ');
    screen += linesNum;
    screen += " ║\n";

    // Spacer
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Sắp xếp thứ hạng
    char rankBuf[64];
    const char* suffix = "th";
    if (rank == 1)      suffix = "st";
    else if (rank == 2) suffix = "nd";
    else if (rank == 3) suffix = "rd";

    snprintf(rankBuf, sizeof(rankBuf), "Your Rank: %d%s", rank, suffix);
    string rankStr(rankBuf);
    int rankPadding = totalWidth - static_cast<int>(rankStr.length());
    int rankLeft    = rankPadding / 2;
    int rankRight   = rankPadding - rankLeft;

    screen += "║";
    screen.append(rankLeft, ' ');
    screen += rankStr;
    screen.append(rankRight, ' ');
    screen += "║\n";

    // Spacer
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Danh sách thứ hạng
    for (size_t i = 0; i < state.highScores.size(); ++i) {
        string suff = "th";
        if (i == 0)      suff = "st";
        else if (i == 1) suff = "nd";
        else if (i == 2) suff = "rd";

        string rankLabel  = to_string(i + 1) + suff;
        string scoreStr   = to_string(state.highScores[i]);
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

    // Spacer
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Restart / Quit prompt
    const string prompt = "Press R to Restart or Q to Quit";
    int promptPadding        = totalWidth - static_cast<int>(prompt.length());
    int promptLeft           = promptPadding / 2;
    int promptRight          = promptPadding - promptLeft;

    screen += "║";
    screen.append(promptLeft, ' ');
    screen += prompt;
    screen.append(promptRight, ' ');
    screen += "║\n";

    // Spacer
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Bottom border
    screen += "╚";
    for (int i = 0; i < totalWidth; ++i) screen += "═";
    screen += "╝\n";

    cout << screen;
    cout.flush();
}

void TetrisGame::resetGame() {
    // Reset game state
    state.running      = true;
    state.paused       = false;
    state.quitByUser   = false;
    state.score        = 0;
    state.level        = 1;
    state.linesCleared = 0;

    board.init();
    dropCounter = 0;

    // Lấy ngẫu nhiên loại block
    uniform_int_distribution<int> dist(0,
        BlockTemplate::NUM_BLOCK_TYPES - 1);
    nextPieceType = dist(rng);
    spawnNewPiece();
}

// \=== Terminal raw mode handling ===

void TetrisGame::enableRawMode() {
    // Lưu lại các cài đặt ban đầu
    tcgetattr(STDIN_FILENO, &origTermios);

    termios raw = origTermios;
    raw.c_lflag &= ~(ICANON | ECHO); // Chuyển sang mode raw
    raw.c_cc[VMIN]  = 0;             // Không đợi ký tự
    raw.c_cc[VTIME] = 0;             // Không đợi thời gian

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // Bật non-blocking mode
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void TetrisGame::disableRawMode() {
    // Khôi phục lại các cài đặt ban đầu
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
                case 'A': return 'w'; // Up    -> Xoay
                case 'B': return 's'; // Down  -> soft drop
                case 'C': return 'd'; // Right -> Di chuyển phải
                case 'D': return 'a'; // Left  -> Di chuyển trái
            }
        }
        return 27;
    }

    return ch;
}

void TetrisGame::flushInput() const {
    // Xóa bất kỳ ký tự đầu vào nào trong buffer terminal
    tcflush(STDIN_FILENO, TCIFLUSH);
}

// \=== Game logic ===

void TetrisGame::animateGameOver() {
    // Chuyển tất cả các block đã khóa thành '#' từ dưới lên để tạo hiệu ứng sóng
    for (int y = BOARD_HEIGHT - 1; y >= 0; --y) {
        bool hasBlock = false;
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            if (board.grid[y][x] != ' ') {
                hasBlock = true;
                board.grid[y][x] = '#';

                string preview[4];
                getNextPiecePreview(preview);
                board.draw(state, preview);

                usleep(ANIM_DELAY_US);
            }
        }

        if (!hasBlock) {
            // Bỏ qua các hàng trống
            continue;
        }
    }

    flushInput();
    usleep(500000); // Dừng 0.5s để người chơi thấy frame cuối cùng
    flushInput();
}

bool TetrisGame::isInsidePlayfield(int x, int y) const {
    // Kiểm tra xem tọa độ có nằm trong playfield không
    return x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT;
}

Piece TetrisGame::calculateGhostPiece() const {
    // Tạo một bản sao của block hiện tại và thả nó xuống cho đến khi va chạm
    Piece ghost = currentPiece;

    bool canMoveDown = true;
    while (canMoveDown) {
        canMoveDown = false;

        // Kiểm tra va chạm với các block đã khóa
        for (int row = 0; row < BlockTemplate::BLOCK_SIZE; ++row) {
            for (int col = 0; col < BlockTemplate::BLOCK_SIZE; ++col) {
                char cell = BlockTemplate::getCell(
                    ghost.type, ghost.rotation, row, col
                );
                if (cell == ' ') continue;

                int xt = ghost.pos.x + col;
                int yt = ghost.pos.y + row + 1; // test next row down

                // Nếu vượt quá giới hạn dưới, dừng lại
                if (yt >= BOARD_HEIGHT) {
                    goto done_checking;
                }

                if (yt >= 0) {
                    // Chỉ va chạm với các block thực tế, không va chạm với các cell của ghost '.'
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
    // Kiểm tra xem block có thể xuất hiện không
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
    // Kiểm tra xem block có thể di chuyển không
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
    // Chỉ xóa các cell đã được đánh dấu là ghost '.'
    for (const Position& pos : lastGhostPositions) {
        if (isInsidePlayfield(pos.x, pos.y) &&
            board.grid[pos.y][pos.x] == '.') {
            board.grid[pos.y][pos.x] = ' ';
        }
    }
    lastGhostPositions.clear();
}

void TetrisGame::placeGhostPiece(const Piece& ghostPiece) {
    // Vẽ outline của ghost block vào các cell trống
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
    // Giống placePiece(true) nhưng không ghi đè các cell không rỗng
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
    // Tạo block mới
    uniform_int_distribution<int> dist(0,
        BlockTemplate::NUM_BLOCK_TYPES - 1);

    Piece spawn;
    spawn.type      = nextPieceType;
    spawn.rotation  = 0;
    int spawnX      = (BOARD_WIDTH / 2) - (BlockTemplate::BLOCK_SIZE / 2);
    spawn.pos       = Position(spawnX, -1);

    currentPiece = spawn;

    if (!canSpawn(spawn)) {
        // Nếu block mới không thể xuất hiện, trò chơi kết thúc
        state.running = false;
        return;
    }

    nextPieceType = dist(rng);
}

bool TetrisGame::lockPieceAndCheck(bool muteLockSound) {
    // Tự động khóa block hiện tại và xóa các hàng nếu cần
    placePiece(currentPiece, true);

    int lines = board.clearLines();
    if (lines > 0) {
        // Nếu có hàng được xóa, chơi âm thanh
        if (lines == 4) {
            SoundManager::play4LinesClearSound();
        } else {
            SoundManager::playLineClearSound();
        }

        state.linesCleared += lines;

        // Tính điểm: điểm cơ bản nhân với cấp độ hiện tại
        const int scores[] = {0, 100, 300, 500, 800};
        state.score += scores[lines] * state.level;

        int oldLevel = state.level;

        // +1 level per 10 lines
        state.level = 1 + (state.linesCleared / LINES_PER_LEVEL);

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
    // Di chuyển block xuống nhanh hơn
    if (canMove(0, 1, currentPiece.rotation)) {
        ++currentPiece.pos.y;
    } else {
        // Kết thúc trò chơi nếu block rơi ra ngoài board
        if (currentPiece.pos.y < 0) {
            state.running = false;
            return;
        }
        state.running = lockPieceAndCheck(true);
        dropCounter   = 0;
    }
}

void TetrisGame::hardDrop() {
    // Di chuyển xuống cho đến khi block va chạm
    while (canMove(0, 1, currentPiece.rotation)) {
        ++currentPiece.pos.y;
    }

    // Kết thúc trò chơi nếu block rơi ra ngoài board
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

    // Bật/tắt pause
    if (c == 'p') {
        state.paused = !state.paused;
        flushInput();
        if (state.paused) {
            drawPauseScreen();
        }
        return;
    }

    // Bật/tắt Ghost Piece
    if (c == 'g') {
        state.ghostEnabled = !state.ghostEnabled;
        return;
    }

    if (state.paused) {
        // Chỉ xử lý 'q' để thoát khi trò chơi bị pause
        if (c == 'q') {
            state.running   = false;
            state.quitByUser = true;
            SoundManager::stopBackgroundSound();
        }
        return;
    }

    // Xử lý các phím gameplay
    switch (c) {
        case 'a': // di chuyển trái
            if (canMove(-1, 0, currentPiece.rotation)) {
                --currentPiece.pos.x;
            }
            break;
        case 'd': // di chuyển phải
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
        case 'w': { // xoay block
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
    // Nếu trò chơi không chạy hoặc bị pause, không xử lý
    if (!state.running || state.paused) return;

    ++dropCounter;

    // Block chỉ rơi xuống 1 hàng sau mỗi DROP_INTERVAL_TICKS lần gọi
    if (dropCounter < DROP_INTERVAL_TICKS) {
        return;
    }

    dropCounter = 0;

    // Kiểm tra xem block có thể rơi xuống 1 hàng không
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

void TetrisGame::getNextPiecePreview(string lines[4]) {
    // Nếu block tiếp theo chưa thay đổi, tái sử dụng preview đã lưu
    if (cachedNextPieceType == nextPieceType) {
        for (int i = 0; i < 4; ++i) {
            lines[i] = cachedNextPiecePreview[i];
        }
        return;
    }

    // Render 4 hàng của template 4x4 thành "██" + khoảng trắng
    for (int row = 0; row < 4; ++row) {
        cachedNextPiecePreview[row].clear();
        cachedNextPiecePreview[row].reserve(64);

        for (int col = 0; col < 4; ++col) {
            char cell = BlockTemplate::getCell(nextPieceType, 0, row, col);
            if (cell != ' ') {
                cachedNextPiecePreview[row] += PIECE_COLORS[nextPieceType];
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
    // Tốc độ rơi của block dựa theo level
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
    // Cập nhật tốc độ rơi theo level
    dropSpeedUs = computeDropSpeedUs(state.level);
}

void TetrisGame::drawPauseScreen() const {
    string screen;
    screen.reserve(1024);

    screen += "\033[2J\033[1;1H";

    int totalWidth = (BOARD_WIDTH * 2) + 13;

    // Top border
    screen += "╔";
    for (int i = 0; i < totalWidth; ++i) screen += "═";
    screen += "╗\n";

    // Some spacer rows
    for (int i = 0; i < 3; ++i) {
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";
    }

    // "GAME PAUSED" title
    const string title = "GAME PAUSED";
    int titlePadding        = totalWidth - static_cast<int>(title.length());
    int titleLeft           = titlePadding / 2;
    int titleRight          = titlePadding - titleLeft;

    screen += "║";
    screen.append(titleLeft, ' ');
    screen += title;
    screen.append(titleRight, ' ');
    screen += "║\n";

    // Spacer
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Score row
    char scoreBuf[64];
    snprintf(scoreBuf, sizeof(scoreBuf), "Score: %d", state.score);
    string scoreStr(scoreBuf);
    int scorePadding = totalWidth - static_cast<int>(scoreStr.length());
    int scoreLeft    = scorePadding / 2;
    int scoreRight   = scorePadding - scoreLeft;

    screen += "║";
    screen.append(scoreLeft, ' ');
    screen += scoreStr;
    screen.append(scoreRight, ' ');
    screen += "║\n";

    // Level row
    char levelBuf[64];
    snprintf(levelBuf, sizeof(levelBuf), "Level: %d", state.level);
    string levelStr(levelBuf);
    int levelPadding = totalWidth - static_cast<int>(levelStr.length());
    int levelLeft    = levelPadding / 2;
    int levelRight   = levelPadding - levelLeft;

    screen += "║";
    screen.append(levelLeft, ' ');
    screen += levelStr;
    screen.append(levelRight, ' ');
    screen += "║\n";

    // Lines row
    char linesBuf[64];
    snprintf(linesBuf, sizeof(linesBuf), "Lines: %d",
                  state.linesCleared);
    string linesStr(linesBuf);
    int linesPadding = totalWidth - static_cast<int>(linesStr.length());
    int linesLeft    = linesPadding / 2;
    int linesRight   = linesPadding - linesLeft;

    screen += "║";
    screen.append(linesLeft, ' ');
    screen += linesStr;
    screen.append(linesRight, ' ');
    screen += "║\n";

    // Spacer
    screen += "║";
    screen.append(totalWidth, ' ');
    screen += "║\n";

    // Options: Resume and Quit
    const string resumeOption = "P - Resume";
    int resumePadding = totalWidth - static_cast<int>(resumeOption.length());
    int resumeLeft    = resumePadding / 2;
    int resumeRight   = resumePadding - resumeLeft;

    screen += "║";
    screen.append(resumeLeft, ' ');
    screen += resumeOption;
    screen.append(resumeRight, ' ');
    screen += "║\n";

    const string quitOption = "Q - Quit";
    int quitPadding = totalWidth - static_cast<int>(quitOption.length());
    int quitLeft    = quitPadding / 2;
    int quitRight   = quitPadding - quitLeft;

    screen += "║";
    screen.append(quitLeft, ' ');
    screen += quitOption;
    screen.append(quitRight, ' ');
    screen += "║\n";

    // Bottom spacers
    for (int i = 0; i < 3; ++i) {
        screen += "║";
        screen.append(totalWidth, ' ');
        screen += "║\n";
    }

    // Bottom border
    screen += "╚";
    for (int i = 0; i < totalWidth; ++i) screen += "═";
    screen += "╝\n";

    cout << screen;
    cout.flush();
}

// \=== Main game loop ===

void TetrisGame::run() {
    BlockTemplate::initializeTemplates();

    bool shouldRestart = true;

    while (shouldRestart) {
        board.init();

        uniform_int_distribution<int> dist(
            0, BlockTemplate::NUM_BLOCK_TYPES - 1
        );
        nextPieceType = dist(rng);

        drawStartScreen();
        waitForKeyPress();

        // Restart background music cleanly
        SoundManager::stopBackgroundSound();
        usleep(100000);
        SoundManager::playBackgroundSound();

        updateDifficulty();
        spawnNewPiece();

        // Core game loop.
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

            string preview[4];
            getNextPiecePreview(preview);
            board.draw(state, preview);

            placePiece(currentPiece, false);

            usleep(dropSpeedUs / DROP_INTERVAL_TICKS);
        }

        if (!state.quitByUser) {
            // Make sure last piece is visible.
            placePieceSafe(currentPiece);

            string preview[4];
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
