#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <random>

using namespace std;

constexpr int BOARD_HEIGHT     = 20;
constexpr int BOARD_WIDTH      = 15;
constexpr int NEXT_PICE_WIDTH  = 14;

constexpr int BLOCK_SIZE       = 4;
constexpr int NUM_BLOCK_TYPES  = 7;

// gameplay tuning
constexpr long BASE_DROP_SPEED_US   = 500000; // base drop speed (µs)
constexpr int  DROP_INTERVAL_TICKS  = 5;      // logic steps per drop

struct Position {
    int x{}, y{};
    Position() = default;
    Position(int _x, int _y) : x(_x), y(_y) {}
};

struct GameState {
    bool running{true};
    bool quitByUser{false};   // Track if user quit manually vs. game over
    bool paused{false};       // Pause state tracking
    int score{0};
    int level{1};
    int linesCleared{0};
};

struct Piece {
    int type{0};
    int rotation{0};
    Position pos{5, 0};
};

struct Board {
    char grid[BOARD_HEIGHT][BOARD_WIDTH]{};

    void init() {
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                grid[i][j] = ' ';
            }
        }
    }

    void draw(const GameState& state, const string nextPieceLines[4]) const {
        string frame;
        frame.reserve(3072);

        // clear screen + move cursor to top-left
        frame += "\033[2J\033[1;1H";

        const string title = "TETRIS GAME";

        // top border
        frame += '+';
        frame.append(BOARD_WIDTH, '-');
        frame += '+';
        frame.append(NEXT_PICE_WIDTH, '-');
        frame += "+\n";

        // title row with "NEXT PIECE"
        frame += '|';
        int totalPadding = BOARD_WIDTH - static_cast<int>(title.size());
        int leftPad = totalPadding / 2;
        int rightPad = totalPadding - leftPad;

        frame.append(leftPad, ' ');
        frame += title;
        frame.append(rightPad, ' ');
        frame += "|  NEXT PIECE  |\n";

        // separator under title
        frame += '+';
        frame.append(BOARD_WIDTH, '-');
        frame += '+';
        frame.append(NEXT_PICE_WIDTH, '-');
        frame += "+\n";

        // board rows + side panel
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            frame += '|';

            for (int j = 0; j < BOARD_WIDTH; ++j) {
                frame += grid[i][j];
            }

            frame += '|';

            if (i == 0) {
                frame += "              |";
            } else if (i >= 1 && i <= 4) {
                frame += "     ";
                frame += nextPieceLines[i - 1];
                frame += "     |";
            } else if (i == 5) {
                frame.append(NEXT_PICE_WIDTH, '-');
                frame += '|';
            } else if (i == 6) {
                char buf[20];
                snprintf(buf, sizeof(buf), " SCORE: %-6d", state.score);
                frame += buf;
                frame += '|';
            } else if (i == 7) {
                char buf[20];
                snprintf(buf, sizeof(buf), " LEVEL: %-6d", state.level);
                frame += buf;
                frame += '|';
            } else if (i == 8) {
                char buf[20];
                snprintf(buf, sizeof(buf), " LINES: %-6d", state.linesCleared);
                frame += buf;
                frame += '|';
            } else {
                frame.append(NEXT_PICE_WIDTH, ' ');
                frame += '|';
            }

            frame += '\n';
        }

        // bottom border
        frame += '+';
        frame.append(BOARD_WIDTH, '-');
        frame += '+';
        frame.append(NEXT_PICE_WIDTH, '-');
        frame += "+\n";

        // controls, updated to include Pause
        frame += "Controls: \u2190\u2192 or A/D (Move)  \u2191/W (Rotate)  \u2193/S (Soft Drop)  X (Soft Drop)  SPACE (Hard Drop)  P (Pause)  Q (Quit)\n";

        cout << frame;
        cout.flush();
    }

    // Draw the pause screen overlay
    void drawPause() const {
        cout << "\033[2J\033[1;1H"; // Clear screen

        cout << "\n\n\n";
        cout << "      ======================\n";
        cout << "      =    GAME PAUSED     =\n";
        cout << "      ======================\n\n";
        cout << "        Press P to Resume   \n";
        cout << "        Press Q to Quit     \n";
        cout.flush();
    }

    int clearLines() {
        int writeRow = BOARD_HEIGHT - 1;
        int linesCleared = 0;

        for (int readRow = BOARD_HEIGHT - 1; readRow >= 0; --readRow) {
            bool full = true;
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                if (grid[readRow][j] == ' ') {
                    full = false;
                    break;
                }
            }

            if (!full) {
                if (writeRow != readRow) {
                    for (int j = 0; j < BOARD_WIDTH; ++j) {
                        grid[writeRow][j] = grid[readRow][j];
                    }
                }
                --writeRow;
            } else {
                ++linesCleared;
            }
        }

        while (writeRow >= 0) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                grid[writeRow][j] = ' ';
            }
            --writeRow;
        }

        return linesCleared;
    }
};

struct BlockTemplate {
    static char templates[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE];

    static void setBlockTemplate(int type,
                                 char symbol,
                                 const int shape[BLOCK_SIZE][BLOCK_SIZE]) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                templates[type][i][j] = shape[i][j] ? symbol : ' ';
            }
        }
    }

    static void initializeTemplates() {
        static const int TETROMINOES[7][4][4] = {
            // I
            {
                {0,1,0,0},
                {0,1,0,0},
                {0,1,0,0},
                {0,1,0,0}
            },
            // O
            {
                {0,0,0,0},
                {0,1,1,0},
                {0,1,1,0},
                {0,0,0,0}
            },
            // T
            {
                {0,0,0,0},
                {0,1,0,0},
                {1,1,1,0},
                {0,0,0,0}
            },
            // S
            {
                {0,0,0,0},
                {0,1,1,0},
                {1,1,0,0},
                {0,0,0,0}
            },
            // Z
            {
                {0,0,0,0},
                {1,1,0,0},
                {0,1,1,0},
                {0,0,0,0}
            },
            // J
            {
                {0,0,0,0},
                {1,0,0,0},
                {1,1,1,0},
                {0,0,0,0}
            },
            // L
            {
                {0,0,0,0},
                {0,0,1,0},
                {1,1,1,0},
                {0,0,0,0}
            }
        };

        static const char NAMES[7] = {'I','O','T','S','Z','J','L'};

        for (int i = 0; i < 7; i++) {
            setBlockTemplate(i, NAMES[i], TETROMINOES[i]);
        }
    }

    // rotation: 0-3 (90 degrees steps clockwise)
    static char getCell(int type, int rotation, int row, int col) {
        int r = row;
        int c = col;

        for (int i = 0; i < rotation; ++i) {
            int temp = 3 - c;
            c = r;
            r = temp;
        }

        return templates[type][r][c];
    }
};

char BlockTemplate::templates[NUM_BLOCK_TYPES][BLOCK_SIZE][BLOCK_SIZE];

struct TetrisGame {
    Board board;
    GameState state;
    Piece currentPiece{};
    int nextPieceType{0};

    termios origTermios{};
    long dropSpeedUs{BASE_DROP_SPEED_US};
    int dropCounter{0};
    bool softDropActive{false};

    mt19937 rng;

    TetrisGame() {
        random_device rd;
        rng.seed(rd());
    }

    void drawStartScreen() {
        string screen;
        screen.reserve(512);

        screen += "\033[2J\033[1;1H";

        int totalWidth = BOARD_WIDTH + NEXT_PICE_WIDTH + 2;

        screen += '+';
        screen.append(totalWidth, '-');
        screen += "+\n";

        screen += '|';
        screen.append(totalWidth, ' ');
        screen += "|\n";

        const string title = "TETRIS GAME";
        int titlePadding = totalWidth - static_cast<int>(title.length());
        int titleLeft = titlePadding / 2;
        int titleRight = titlePadding - titleLeft;

        screen += '|';
        screen.append(titleLeft, ' ');
        screen += title;
        screen.append(titleRight, ' ');
        screen += "|\n";

        screen += '|';
        screen.append(totalWidth, ' ');
        screen += "|\n";

        const string prompt = "Press any key to start...";
        int promptPadding = totalWidth - static_cast<int>(prompt.length());
        int promptLeft = promptPadding / 2;
        int promptRight = promptPadding - promptLeft;

        screen += '|';
        screen.append(promptLeft, ' ');
        screen += prompt;
        screen.append(promptRight, ' ');
        screen += "|\n";

        screen += '|';
        screen.append(totalWidth, ' ');
        screen += "|\n";

        screen += '+';
        screen.append(totalWidth, '-');
        screen += "+\n";

        cout << screen;
        cout.flush();
    }

    char waitForKeyPress() {
        enableRawMode();

        char key = 0;
        while ((key = getInput()) == 0) {
            usleep(50000);
        }

        flushInput();

        return key;
    }

    void drawGameOverScreen(int rank) {
        string screen;
        screen.reserve(1024);

        screen += "\033[2J\033[1;1H";

        int totalWidth = BOARD_WIDTH + NEXT_PICE_WIDTH + 2;

        screen += '+';
        screen.append(totalWidth, '-');
        screen += "+\n";

        screen += '|';
        screen.append(totalWidth, ' ');
        screen += "|\n";

        const string title = "GAME OVER";
        int titlePadding = totalWidth - static_cast<int>(title.length());
        int titleLeft = titlePadding / 2;
        int titleRight = titlePadding - titleLeft;

        screen += '|';
        screen.append(titleLeft, ' ');
        screen += title;
        screen.append(titleRight, ' ');
        screen += "|\n";

        screen += '|';
        screen.append(totalWidth, ' ');
        screen += "|\n";

        char scoreBuf[64];
        snprintf(scoreBuf, sizeof(scoreBuf), "Final Score: %d", state.score);
        string scoreStr(scoreBuf);
        int scorePadding = totalWidth - static_cast<int>(scoreStr.length());
        int scoreLeft = scorePadding / 2;
        int scoreRight = scorePadding - scoreLeft;

        screen += '|';
        screen.append(scoreLeft, ' ');
        screen += scoreStr;
        screen.append(scoreRight, ' ');
        screen += "|\n";

        char levelBuf[64];
        snprintf(levelBuf, sizeof(levelBuf), "Level: %d", state.level);
        string levelStr(levelBuf);
        int levelPadding = totalWidth - static_cast<int>(levelStr.length());
        int levelLeft = levelPadding / 2;
        int levelRight = levelPadding - levelLeft;

        screen += '|';
        screen.append(levelLeft, ' ');
        screen += levelStr;
        screen.append(levelRight, ' ');
        screen += "|\n";

        char linesBuf[64];
        snprintf(linesBuf, sizeof(linesBuf), "Lines Cleared: %d", state.linesCleared);
        string linesStr(linesBuf);
        int linesPadding = totalWidth - static_cast<int>(linesStr.length());
        int linesLeft = linesPadding / 2;
        int linesRight = linesPadding - linesLeft;

        screen += '|';
        screen.append(linesLeft, ' ');
        screen += linesStr;
        screen.append(linesRight, ' ');
        screen += "|\n";

        screen += '|';
        screen.append(totalWidth, ' ');
        screen += "|\n";

        char rankBuf[64];
        const char* suffix = "th";
        if (rank == 1) suffix = "st";
        else if (rank == 2) suffix = "nd";
        else if (rank == 3) suffix = "rd";
        snprintf(rankBuf, sizeof(rankBuf), "Your Rank: %d%s", rank, suffix);
        string rankStr(rankBuf);
        int rankPadding = totalWidth - static_cast<int>(rankStr.length());
        int rankLeft = rankPadding / 2;
        int rankRight = rankPadding - rankLeft;

        screen += '|';
        screen.append(rankLeft, ' ');
        screen += rankStr;
        screen.append(rankRight, ' ');
        screen += "|\n";

        screen += '|';
        screen.append(totalWidth, ' ');
        screen += "|\n";

        const string prompt = "Press any key to quit";
        int promptPadding = totalWidth - static_cast<int>(prompt.length());
        int promptLeft = promptPadding / 2;
        int promptRight = promptPadding - promptLeft;

        screen += '|';
        screen.append(promptLeft, ' ');
        screen += prompt;
        screen.append(promptRight, ' ');
        screen += "|\n";

        screen += '|';
        screen.append(totalWidth, ' ');
        screen += "|\n";

        screen += '+';
        screen.append(totalWidth, '-');
        screen += "+\n";

        cout << screen;
        cout.flush();
    }

    // ---------- terminal handling (POSIX) ----------

    void enableRawMode() {
        tcgetattr(STDIN_FILENO, &origTermios);

        termios raw = origTermios;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;

        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    void disableRawMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios);
    }

    char getInput() const {
        char ch = 0;
        ssize_t result = read(STDIN_FILENO, &ch, 1);
        if (result <= 0) return 0;

        if (ch == 27) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) return 27;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) return 27;

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': return 'w';
                    case 'B': return 's';
                    case 'C': return 'd';
                    case 'D': return 'a';
                }
            }
            return 27;
        }

        return ch;
    }

    void flushInput() const {
        tcflush(STDIN_FILENO, TCIFLUSH);
    }

    // ---------- helpers for spawn / movement ----------

    bool isInsidePlayfield(int x, int y) const {
        return x >= 0 && x < BOARD_WIDTH &&
               y >= 0 && y < BOARD_HEIGHT;
    }

    bool canSpawn(const Piece& piece) const {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(piece.type, piece.rotation, i, j);
                if (cell == ' ') continue;

                int xt = piece.pos.x + j;
                int yt = piece.pos.y + i;

                if (xt < 0 || xt >= BOARD_WIDTH) return false;
                if (yt >= BOARD_HEIGHT) return false;

                if (yt >= 0) {
                    char gridCell = board.grid[yt][xt];
                    if (gridCell != ' ') {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool canMove(int dx, int dy, int newRotation) const {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(
                    currentPiece.type, newRotation, i, j
                );
                if (cell == ' ') continue;

                int xt = currentPiece.pos.x + j + dx;
                int yt = currentPiece.pos.y + i + dy;

                if (xt < 0 || xt >= BOARD_WIDTH) return false;
                if (yt >= BOARD_HEIGHT) return false;

                if (yt >= 0) {
                    char gridCell = board.grid[yt][xt];
                    if (gridCell != ' ') {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void placePiece(const Piece& piece, bool place) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(
                    piece.type, piece.rotation, i, j
                );
                if (cell == ' ') continue;

                int xt = piece.pos.x + j;
                int yt = piece.pos.y + i;

                if (yt < 0 || yt >= BOARD_HEIGHT ||
                    xt < 0 || xt >= BOARD_WIDTH) {
                    continue;
                }

                board.grid[yt][xt] = place ? cell : ' ';
            }
        }
    }

    void placePieceSafe(const Piece& piece) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(
                    piece.type, piece.rotation, i, j
                );
                if (cell == ' ') continue;

                int xt = piece.pos.x + j;
                int yt = piece.pos.y + i;

                if (yt < 0 || yt >= BOARD_HEIGHT ||
                    xt < 0 || xt >= BOARD_WIDTH) {
                    continue;
                }

                if (board.grid[yt][xt] == ' ') {
                    board.grid[yt][xt] = cell;
                }
            }
        }
    }

    void spawnNewPiece() {
        uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);

        Piece testPiece;
        testPiece.type = nextPieceType;
        testPiece.rotation = 0;

        int spawnX = (BOARD_WIDTH / 2) - (BLOCK_SIZE / 2);
        testPiece.pos = Position(spawnX, -1);

        currentPiece = testPiece;

        if (!canSpawn(testPiece)) {
            state.running = false;
            return;
        }

        nextPieceType = dist(rng);
    }

    bool lockPieceAndCheck() {
        placePiece(currentPiece, true);

        int lines = board.clearLines();
        if (lines > 0) {
            state.linesCleared += lines;

            // Scoring rules
            const int scores[] = {0, 100, 300, 500, 800};
            state.score += scores[lines] * state.level;

            // Level progression: +1 level per 10 lines
            state.level = 1 + (state.linesCleared / 10);

            // Update falling speed based on new level
            updateDifficulty();
        }

        spawnNewPiece();
        return state.running;
    }

    void softDrop() {
        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            if (currentPiece.pos.y < 0) {
                state.running = false;
                return;
            }
            state.running = lockPieceAndCheck();
            dropCounter = 0;
        }
    }

    void hardDrop() {
        while (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        }
        if (currentPiece.pos.y < 0) {
            state.running = false;
            return;
        }
        state.running = lockPieceAndCheck();
        dropCounter = 0;
    }

    void handleInput() {
        char c = getInput();

        if (c == 's') {
            softDropActive = true;
        } else {
            softDropActive = false;
        }

        if (c == 0) return;

        // Handle 'P' key for Pause toggle
        if (c == 'p') {
            state.paused = !state.paused;
            if (state.paused) {
                board.drawPause(); // Draw pause screen immediately
            }
            return;
        }

        // If paused, block all inputs except 'Q' and 'P'
        if (state.paused) {
            if (c == 'q') { // Allow quitting while paused
                state.running = false;
            }
            return; // Skip movement logic below
        }

        switch (c) {
            case 'a':
                if (canMove(-1, 0, currentPiece.rotation)) {
                    currentPiece.pos.x--;
                }
                break;
            case 'd':
                if (canMove(1, 0, currentPiece.rotation)) {
                    currentPiece.pos.x++;
                }
                break;
            case 's':
                // `s` enables soft drop via softDropActive above
                break;
            case 'x':
                softDrop();
                break;
            case ' ':
                hardDrop();
                flushInput();
                break;
            case 'w': {
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
            case 'q':
                state.running = false;
                state.quitByUser = true;
                break;
            default:
                break;
        }
    }

    void handleGravity() {
        if (!state.running) return;
        // Do not process gravity if game is paused
        if (state.paused) return;

        ++dropCounter;

        int effectiveInterval = softDropActive ? 1 : DROP_INTERVAL_TICKS;

        if (dropCounter < effectiveInterval) return;

        dropCounter = 0;

        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            if (currentPiece.pos.y < 0) {
                state.running = false;
                return;
            }
            state.running = lockPieceAndCheck();
        }
    }

    void getNextPiecePreview(string lines[4]) const {
        for (int row = 0; row < 4; ++row) {
            lines[row].clear();
            for (int col = 0; col < 4; ++col) {
                char cell = BlockTemplate::getCell(nextPieceType, 0, row, col);
                lines[row] += cell;
            }
        }
    }

    void run() {
        BlockTemplate::initializeTemplates();

        board.init();

        uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);
        nextPieceType = dist(rng);

        drawStartScreen();
        waitForKeyPress();

        // initialize speed for starting level
        updateDifficulty();
        spawnNewPiece();

        while (state.running) {
            handleInput();

            // If paused, skip rendering and gravity logic to save CPU
            if (state.paused) {
                usleep(100000); // Sleep for 100ms
                continue;
            }

            if (!state.running) break;

            handleGravity();

            placePiece(currentPiece, true);

            string preview[4];
            getNextPiecePreview(preview);
            board.draw(state, preview);

            placePiece(currentPiece, false);

            usleep(dropSpeedUs / DROP_INTERVAL_TICKS);
        }

        if (!state.quitByUser) {
            placePieceSafe(currentPiece);

            string preview[4];
            getNextPiecePreview(preview);
            board.draw(state, preview);

            flushInput();
            usleep(800000);
            flushInput();
        }

        int rank = 1; // no file I/O, dummy rank
        drawGameOverScreen(rank);

        waitForKeyPress(); // wait then quit

        disableRawMode();
    }

    long computeDropSpeedUs(int level) const {
        if (level <= 3) {            // Levels 1–3: Slow
            return 500000;           // 0.50s per tick group
        } else if (level <= 6) {     // Levels 4–6: Medium
            return 300000;           // 0.30s
        } else if (level <= 9) {     // Levels 7–9: Fast
            return 150000;           // 0.15s
        } else {                     // Level 10+
            return 80000;            // 0.08s
        }
    }

    void updateDifficulty() {
        dropSpeedUs = computeDropSpeedUs(state.level);
    }
};

int main() {
    TetrisGame game;
    game.run();
    return 0;
}
