#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <random>

using namespace std;

constexpr int BOARD_HEIGHT      = 20;
constexpr int BOARD_WIDTH       = 15;
constexpr int BLOCK_SIZE        = 4;
constexpr int NUM_BLOCK_TYPES   = 7;
const string FILE_NAME = "highscore.txt";

// gameplay tuning
constexpr long BASE_DROP_SPEED_US   = 500000; // base drop speed (0.5s)
constexpr int  DROP_INTERVAL_TICKS  = 5;      // logic steps per drop

struct Position {
    int x{}, y{};
    Position() = default;
    Position(int _x, int _y) : x(_x), y(_y) {}
};

struct GameState {
    bool running{true};
    bool paused{false};
    
    // [NEW] Stats for the game
    int score{0};
    int highScore{0}; // Variable exists, but lives in RAM only

    int level{1};
    int lines{0};
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
                grid[i][j] = (i == 0 || i == BOARD_HEIGHT - 1 ||
                              j == 0 || j == BOARD_WIDTH - 1)
                              ? '#'
                              : ' ';
            }
        }
    }

    void draw(const GameState& state) const {
        // clear screen + move cursor to top-left
        cout << "\033[2J\033[1;1H";
        
        // [UPDATE] Display Stats
        cout << "Tetris - Score: " << state.score 
             << " | Lines: " << state.lines 
             << " | Level: " << state.level << "\n";
             
        // [UPDATE] Added 'R' to controls
        cout << "Controls: A/D=Move  W=Rotate  SPACE=Drop  P=Pause  R=Restart  Q=Quit\n\n";

        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                cout << grid[i][j];
            }
            cout << "\n";
        }
        cout.flush();
    }

    void drawPause() const {
        cout << "\033[2J\033[1;1H";
        cout << "\n\n\n";
        cout << "      ======================\n";
        cout << "      =    GAME PAUSED     =\n";
        cout << "      ======================\n\n";
        cout << "        Press P to Resume   \n";
        cout << "        Press R to Restart  \n"; // Updated menu
        cout << "        Press Q to Quit     \n";
        cout.flush();
    }

    // Updated to return number of lines cleared (for scoring)
    int clearLines() {
        int linesCleared = 0;
        int writeRow = BOARD_HEIGHT - 2;

        // Scan from bottom to top
        for (int readRow = BOARD_HEIGHT - 2; readRow > 0; --readRow) {
            bool full = true;
            for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
                if (grid[readRow][j] == ' ') {
                    full = false;
                    break;
                }
            }

            // Keep non-full rows, skip full ones
            if (!full) {
                if (writeRow != readRow) {
                    for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
                        grid[writeRow][j] = grid[readRow][j];
                    }
                }
                --writeRow;
            } else {
                linesCleared++;
            }
        }

        // Clear remaining top rows
        while (writeRow > 0) {
            for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
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
            { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} },
            // O
            { {0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0} },
            // T
            { {0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0} },
            // S
            { {0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0} },
            // Z
            { {0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0} },
            // J
            { {0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0} },
            // L
            { {0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0} }
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
    long dropSpeedUs{BASE_DROP_SPEED_US};
    int dropCounter{0};

    std::mt19937 rng;

    TetrisGame() {
        std::random_device rd;
        rng.seed(rd());
    }

    // Logic to update (inside lockPieceAndCheck):
    void updateScore() {
    // Simple check: Is current score better than best?
    if (state.score > state.highScore) {
        state.highScore = state.score;
        // PROBLEM: This data is lost when the program terminates.
    }
    // 1. Loading (Read one integer)
    void loadHighScore() {
        ifstream file(FILE_NAME);
        if (file.is_open()) {
            file >> state.highScore; // Only reads the first number
            file.close();
        }
    }

    // 2. Saving (Overwrite mode)
    void saveHighScore() {
        // Only save if we beat the previous record
        if (state.score > state.highScore) {
            state.highScore = state.score;
            
            ofstream file(FILE_NAME); // Default mode: OVERWRITES everything
            if (file.is_open()) {
                file << state.highScore;
                file.close();
            }
        }
    }
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
        return (result > 0) ? ch : 0;
    }

    void flushInput() const {
        char ch;
        tcflush(STDIN_FILENO, TCIFLUSH);
    }

    // ---------- helpers for spawn / movement ----------

    bool isInsidePlayfield(int x, int y) const {
        return x >= 1 && x < BOARD_WIDTH - 1 &&
               y >= 0 && y < BOARD_HEIGHT - 1;
    }

    bool canSpawn(const Piece& piece) const {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                char cell = BlockTemplate::getCell(piece.type, piece.rotation, i, j);
                if (cell == ' ') continue;

                int xt = piece.pos.x + j;
                int yt = piece.pos.y + i;

                if (yt >= 0 && !isInsidePlayfield(xt, yt)) {
                    return false;
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

                if (xt < 1 || xt >= BOARD_WIDTH - 1) return false;
                if (yt >= BOARD_HEIGHT - 1) return false;
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

                if (yt < 0 || yt >= BOARD_HEIGHT || xt < 0 || xt >= BOARD_WIDTH) continue;

                board.grid[yt][xt] = place ? cell : ' ';
            }
        }
    }

    void spawnNewPiece() {
        std::uniform_int_distribution<int> dist(0, NUM_BLOCK_TYPES - 1);
        currentPiece.type = dist(rng);
        currentPiece.rotation = 0;
        int spawnX = (BOARD_WIDTH / 2) - (BLOCK_SIZE / 2);
        currentPiece.pos = Position(spawnX, -1);

        if (!canSpawn(currentPiece)) {
            state.running = false;
        }
    }
    
    // [NEW] Feature: Restart Game Logic
    // Resets everything to initial state
    void resetGame() {
        // 1. Reset Board
        board.init(); 
        
        // 2. Reset Stats
        state.score = 0;
        state.lines = 0;
        state.level = 1;
        
        // 3. Reset State
        state.running = true;
        state.paused = false;
        
        // 4. Reset Timers
        dropSpeedUs = BASE_DROP_SPEED_US;
        dropCounter = 0;
        
        // 5. Generate new piece
        spawnNewPiece();
        
        // 6. Visual feedback (flash/clear screen)
        cout << "\033[2J\033[1;1H"; 
        cout << ">>> GAME RESTARTED <<<";
        cout.flush();
        usleep(500000); // Small delay to let user see "Restarted"
    }

    bool lockPieceAndCheck() {
        placePiece(currentPiece, true);
        
        // Update stats
        int lines = board.clearLines();
        if (lines > 0) {
            state.lines += lines;
            state.score += (lines * 100 * state.level);
            // Simple level up logic every 5 lines
            if (state.lines / 5 > state.level - 1) {
                 state.level++;
                 dropSpeedUs = max(100000L, dropSpeedUs - 50000); // Increase speed
            }
        }

        spawnNewPiece();
        if (!state.running) {
            // Draw one last time to show where you died
            board.draw(state);
            cout << "\n*** GAME OVER ***\nPress R to Restart or Q to Quit\n";
            return false;
        }
        return true;
    }

    void softDrop() {
        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            state.running = lockPieceAndCheck();
            dropCounter = 0;
        }
    }

    void hardDrop() {
        while (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        }
        state.running = lockPieceAndCheck();
        dropCounter = 0;
    }

    void handleInput() {
        char c = getInput();
        if (c == 0) return;

        // [NEW] Check Restart Key 'R' (Global check)
        if (c == 'r') {
            resetGame();
            return;
        }

        if (c == 'p') {
            state.paused = !state.paused;
            if (state.paused) board.drawPause();
            return;
        }

        if (state.paused) {
            if (c == 'q') state.running = false;
            return;
        }

        // Gameplay controls
        switch (c) {
            case 'a': 
                if (canMove(-1, 0, currentPiece.rotation)) currentPiece.pos.x--; 
                break;
            case 'd': 
                if (canMove(1, 0, currentPiece.rotation)) currentPiece.pos.x++; 
                break;
            case 'x': softDrop(); break;
            case ' ': hardDrop(); flushInput(); break;
            case 'w': {
                int newRot = (currentPiece.rotation + 1) % 4;
                if (canMove(0, 0, newRot)) currentPiece.rotation = newRot;
                // Add simple wall kicks for boundaries
                else if (canMove(-1, 0, newRot)) { currentPiece.pos.x--; currentPiece.rotation = newRot; }
                else if (canMove(1, 0, newRot)) { currentPiece.pos.x++; currentPiece.rotation = newRot; }
                break;
            }
            case 'q': state.running = false; break;
        }
    }

    void handleGravity() {
        if (!state.running || state.paused) return;

        ++dropCounter;
        if (dropCounter < DROP_INTERVAL_TICKS) return;

        dropCounter = 0;
        if (canMove(0, 1, currentPiece.rotation)) {
            currentPiece.pos.y++;
        } else {
            state.running = lockPieceAndCheck();
        }
    }

    void run() {
        BlockTemplate::initializeTemplates();
        board.init();
        
        // Clean start
        for (int y = 1; y < BOARD_HEIGHT - 1; ++y) {
            for (int x = 1; x < BOARD_WIDTH - 1; ++x) {
                board.grid[y][x] = ' ';
            }
        }

        enableRawMode();
        spawnNewPiece();

        cout << "Tetris Game - Starting...\n";
        usleep(500000);

        // [UPDATE] Game Loop Logic
        // Changed loop condition to allow restarting after Game Over
        // We use an outer loop or just let 'state.running' handle it.
        // But since 'lockPieceAndCheck' sets running=false on death, 
        // we need to keep the process alive to catch 'R'.
        
        bool appRunning = true;
        while (appRunning) {
            
            // Sub-loop: Actual Gameplay
            while (state.running) {
                handleInput();

                if (state.paused) {
                    usleep(100000);
                    continue;
                }

                handleGravity();
                placePiece(currentPiece, true);
                board.draw(state);
                placePiece(currentPiece, false);

                usleep(dropSpeedUs / DROP_INTERVAL_TICKS);
            }
            
            // Sub-loop: Game Over State
            // Wait for user to Quit (Q) or Restart (R)
            char c = getInput();
            if (c == 'q') {
                appRunning = false;
            } else if (c == 'r') {
                resetGame(); // This sets state.running = true, loop restarts
            }
            usleep(100000);
        }

        disableRawMode();
        cout << "Thanks for playing!\n";
    }
};

int main() {
    TetrisGame game;
    game.run();
    return 0;
}
