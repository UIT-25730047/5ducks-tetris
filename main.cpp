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

struct Position {
    int x{}, y{};
    Position() = default;
    Position(int _x, int _y) : x(_x), y(_y) {}
};

struct GameState {
    bool running{true};
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
        // Build entire frame in a string buffer for single output
        string frame;
        frame.reserve(3072); // Pre-allocate

        // Clear screen + move cursor to top-left
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

        // Draw board rows with borders
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            // Left border
            frame += '|';
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                frame += grid[i][j];
            }
            // Right border + preview panel
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
    termios origTermios{};
    long dropSpeedUs{500000};
    int dropCounter{0};

    std::mt19937 rng;

    TetrisGame() {
        std::random_device rd;
        rng.seed(rd());
    }

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
        return (result > 0) ? ch : 0;
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

    void getNextPiecePreview(string lines[4]) const {
        // Render the next piece as 4 lines of 4 characters each
        for (int row = 0; row < 4; ++row) {
            lines[row] = "";
            for (int col = 0; col < 4; ++col) {
                char cell = BlockTemplate::getCell(nextPieceType, 0, row, col);
                lines[row] += cell;
            }
        }
    }

    bool isInsidePlayfield(int x, int y) const {
        return x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT;
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

                if (yt < 0 || yt >= BOARD_HEIGHT ||
                    xt < 0 || xt >= BOARD_WIDTH) {
                    continue;
                }
                board.grid[yt][xt] = place ? cell : ' ';
            }
        }
    }

    void spawnNewPiece() {
        std::uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);

        // Create piece for this spawn
        currentPiece.type = dist(rng);
        currentPiece.rotation = 0;

        int spawnX = (BOARD_WIDTH / 2) - (BLOCK_SIZE / 2);
        currentPiece.pos = Position(spawnX, -1);
    }

    bool lockPieceAndCheck() {
        placePiece(currentPiece, true);
        spawnNewPiece();
        return true;
    }

    void softDrop() {
        if (canMove(0, 1, currentPiece.rotation)) currentPiece.pos.y++;
        else { lockPieceAndCheck(); dropCounter = 0;}
    }

    void hardDrop() {
        while (canMove(0, 1, currentPiece.rotation)) currentPiece.pos.y++;
        lockPieceAndCheck();
        dropCounter = 0;
    }

    void handleInput(bool& wantRestart, bool& wantQuit) {
        char c = getInput();
        if (c == 0) return;
        switch (c) {
            case 'a': if (canMove(-1, 0, currentPiece.rotation)) currentPiece.pos.x--; break;
            case 'd': if (canMove(1, 0, currentPiece.rotation)) currentPiece.pos.x++; break;
            case 'x': softDrop(); break;
            case ' ': hardDrop(); flushInput(); break;
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
            case 'q': state.running = false; break;
            case 'r': wantRestart = true; break;
            default: break;
        }
        if (c == 'q') wantQuit = true;
    }

    void handleGravity() {
        if (!state.running) return;
        ++dropCounter;
        if (dropCounter < 5) return;
        dropCounter = 0;
        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            lockPieceAndCheck();
        }
    }

    void run() {
        BlockTemplate::initializeTemplates();
        board.init();

        drawStartScreen();
        waitForKeyPress();

        spawnNewPiece();
        state.running = true;

        while (state.running) {
            bool wantRestart = false, wantQuit = false;
            handleInput(wantRestart, wantQuit);
            if (!state.running) break;
            handleGravity();

            // Draw everything
            placePiece(currentPiece, true);
            board.draw(state);
            placePiece(currentPiece, false);

            usleep(dropSpeedUs / 5);

            if (wantRestart || wantQuit) {
                state.running = false;
                break;
            }
        }

        char choice = waitForKeyPress();
        if (choice == 'r' || choice == 'R') {
            run();
        }
    }
};

int main() {
    TetrisGame game;
    game.run();
    return 0;
}