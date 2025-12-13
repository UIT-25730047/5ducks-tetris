#include <iostream>
#include <string>
#include <random>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

using namespace std;

constexpr int BOARD_HEIGHT     = 20;
constexpr int BOARD_WIDTH      = 15;
constexpr int NEXT_PICE_WIDTH  = 14;

constexpr int BLOCK_SIZE       = 4;
constexpr int NUM_BLOCK_TYPES  = 7;

// gameplay timing
constexpr long BASE_DROP_SPEED_US   = 500000;
constexpr int  DROP_INTERVAL_TICKS  = 5;

struct Position {
    int x{}, y{};
    Position() = default;
    Position(int _x, int _y) : x(_x), y(_y) {}
};

struct GameState {
    bool running{true};
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

        frame += "\033[2J\033[1;1H";
        const string title = "TETRIS GAME";

        // Top border
        frame += '+';
        frame.append(BOARD_WIDTH, '-');
        frame += '+';
        frame.append(NEXT_PICE_WIDTH, '-');
        frame += "+\n";

        // Title row
        frame += '|';
        int totalPadding = BOARD_WIDTH - title.size();
        int leftPad = totalPadding / 2;
        int rightPad = totalPadding - leftPad;

        frame.append(leftPad, ' ');
        frame += title;
        frame.append(rightPad, ' ');
        frame += "|  NEXT PIECE  |\n";

        // Divider
        frame += '+';
        frame.append(BOARD_WIDTH, '-');
        frame += '+';
        frame.append(NEXT_PICE_WIDTH, '-');
        frame += "+\n";

        // Board rows
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            frame += '|';
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                frame += grid[i][j];
            }
            frame += '|';

            // Right panel
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

        // Bottom border
        frame += '+';
        frame.append(BOARD_WIDTH, '-');
        frame += '+';
        frame.append(NEXT_PICE_WIDTH, '-');
        frame += "+\n";

        frame += "Controls: a=left d=right w=rotate x=soft-drop SPACE=hard-drop q=quit\n";

        cout << frame;
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

    static char getCell(int type, int rotation, int row, int col) {
        int r = row, c = col;
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

    std::mt19937 rng;

    TetrisGame() {
        std::random_device rd;
        rng.seed(rd());
    }

    // POSIX terminal helpers
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
    void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios); }
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
    void flushInput() const { tcflush(STDIN_FILENO, TCIFLUSH); }

    void drawStartScreen() {
        string screen;
        screen.reserve(512);

        screen += "\033[2J\033[1;1H";
        int totalWidth = BOARD_WIDTH + NEXT_PICE_WIDTH + 2;
        screen += '+';
        screen.append(totalWidth, '-');
        screen += "+\n|";
        screen.append(totalWidth, ' ');
        screen += "|\n|";
        const string title = "TETRIS GAME";
        int titlePadding = totalWidth - title.length();
        int titleLeft = titlePadding / 2;
        int titleRight = titlePadding - titleLeft;
        screen.append(titleLeft, ' ');
        screen += title;
        screen.append(titleRight, ' ');
        screen += "|\n|";
        screen.append(totalWidth, ' ');
        screen += "|\n|";
        const string prompt = "Press any key to start...";
        int promptPadding = totalWidth - prompt.length();
        int promptLeft = promptPadding / 2;
        int promptRight = promptPadding - promptLeft;
        screen.append(promptLeft, ' ');
        screen += prompt;
        screen.append(promptRight, ' ');
        screen += "|\n|";
        screen.append(totalWidth, ' ');
        screen += "|\n+";
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

    void drawGameOverScreen() {
        string screen;
        screen.reserve(512);

        screen += "\033[2J\033[1;1H";
        int totalWidth = BOARD_WIDTH + NEXT_PICE_WIDTH + 2;
        screen += '+';
        screen.append(totalWidth, '-');
        screen += "+\n|";
        screen.append(totalWidth, ' ');
        screen += "|\n|";
        const string title = "GAME OVER";
        int titlePadding = totalWidth - title.length();
        int titleLeft = titlePadding / 2;
        int titleRight = titlePadding - titleLeft;
        screen.append(titleLeft, ' ');
        screen += title;
        screen.append(titleRight, ' ');
        screen += "|\n|";
        screen.append(totalWidth, ' ');
        screen += "|\n|";
        char scoreBuf[64];
        snprintf(scoreBuf, sizeof(scoreBuf), "Final Score: %d", state.score);
        string scoreStr(scoreBuf);
        int scorePadding = totalWidth - scoreStr.length();
        int scoreLeft = scorePadding / 2;
        int scoreRight = scorePadding - scoreLeft;
        screen.append(scoreLeft, ' ');
        screen += scoreStr;
        screen.append(scoreRight, ' ');
        screen += "|\n|";
        char levelBuf[64];
        snprintf(levelBuf, sizeof(levelBuf), "Level: %d", state.level);
        string levelStr(levelBuf);
        int levelPadding = totalWidth - levelStr.length();
        int levelLeft = levelPadding / 2;
        int levelRight = levelPadding - levelLeft;
        screen.append(levelLeft, ' ');
        screen += levelStr;
        screen.append(levelRight, ' ');
        screen += "|\n|";
        char linesBuf[64];
        snprintf(linesBuf, sizeof(linesBuf), "Lines Cleared: %d", state.linesCleared);
        string linesStr(linesBuf);
        int linesPadding = totalWidth - linesStr.length();
        int linesLeft = linesPadding / 2;
        int linesRight = linesPadding - linesLeft;
        screen.append(linesLeft, ' ');
        screen += linesStr;
        screen.append(linesRight, ' ');
        screen += "|\n|";
        screen.append(totalWidth, ' ');
        screen += "|\n+";
        screen.append(totalWidth, '-');
        screen += "+\n";

        cout << screen;
        cout.flush();
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
                    if (gridCell != ' ') return false;
                }
            }
        }
        return true;
    }

    bool canMove(int dx, int dy, int newRotation) const {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(currentPiece.type, newRotation, i, j);
                if (cell == ' ') continue;

                int xt = currentPiece.pos.x + j + dx;
                int yt = currentPiece.pos.y + i + dy;

                if (xt < 0 || xt >= BOARD_WIDTH) return false;
                if (yt >= BOARD_HEIGHT) return false;
                if (yt >= 0 && board.grid[yt][xt] != ' ') return false;
            }
        }
        return true;
    }

    void placePiece(const Piece& piece, bool place) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(piece.type, piece.rotation, i, j);
                if (cell == ' ') continue;

                int xt = piece.pos.x + j;
                int yt = piece.pos.y + i;

                if (yt < 0 || yt >= BOARD_HEIGHT || xt < 0 || xt >= BOARD_WIDTH) {
                    continue;
                }
                board.grid[yt][xt] = place ? cell : ' ';
            }
        }
    }

    int clearLines() { return board.clearLines(); }

    void spawnNewPiece() {
        std::uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);

        Piece testPiece;
        testPiece.type = nextPieceType;
        testPiece.rotation = 0;
        int spawnX = (BOARD_WIDTH / 2) - (BLOCK_SIZE / 2);
        testPiece.pos = Position(spawnX, -1);

        // Update current piece for rendering
        currentPiece = testPiece;

        // If spawn invalid, end game
        if (!canSpawn(testPiece)) {
            state.running = false;
            return;
        }

        // Prepare next piece
        nextPieceType = dist(rng);
    }

    bool lockPieceAndCheck() {
        placePiece(currentPiece, true);

        int lines = clearLines();
        if (lines > 0) {
            state.linesCleared += lines;
            const int scores[] = {0, 40, 100, 300, 1200};
            state.score += scores[lines] * state.level;
            state.level = 1 + (state.linesCleared / 10);
        }

        spawnNewPiece();
        return state.running;
    }

    void softDrop() {
        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            // If piece still above board, game over
            if (currentPiece.pos.y < 0) {
                state.running = false;
                return;
            }
            state.running = lockPieceAndCheck();
            dropCounter = 0;
        }
    }

    void hardDrop() {
        while (canMove(0, 1, currentPiece.rotation)) currentPiece.pos.y++;
        if (currentPiece.pos.y < 0) { state.running = false; return; }
        state.running = lockPieceAndCheck();
        dropCounter = 0;
    }

    void handleInput() {
        char c = getInput();
        if (c == 0) return;

        switch (c) {
            case 'a':
                if (canMove(-1, 0, currentPiece.rotation)) currentPiece.pos.x--;
                break;
            case 'd':
                if (canMove(1, 0, currentPiece.rotation)) currentPiece.pos.x++;
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
                int kicks[] = {0, -1, 1, -2, 2};
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
                break;
            default:
                break;
        }
    }

    void handleGravity() {
        if (!state.running) return;
        ++dropCounter;
        if (dropCounter < DROP_INTERVAL_TICKS) return;
        dropCounter = 0;

        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            if (currentPiece.pos.y < 0) { state.running = false; return; }
            state.running = lockPieceAndCheck();
        }
    }

    void run() {
        BlockTemplate::initializeTemplates();
        board.init();

        std::uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);
        nextPieceType = dist(rng);

        drawStartScreen();
        waitForKeyPress();

        spawnNewPiece();
        state.running = true;

        while (state.running) {
            handleInput();
            if (!state.running) break;

            handleGravity();

            placePiece(currentPiece, true);
            string preview[4];
            getNextPiecePreview(preview);
            board.draw(state, preview);
            placePiece(currentPiece, false);

            usleep(dropSpeedUs / DROP_INTERVAL_TICKS);
        }

        // Simple game over screen (no animation, no restart)
        drawGameOverScreen();
        disableRawMode();
    }
};

int main() {
    TetrisGame game;
    game.run();
    return 0;
}
