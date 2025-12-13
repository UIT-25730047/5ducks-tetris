#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <random>
#include <fstream> 
#include <vector>    // [NEW] For dynamic array
#include <algorithm> // [NEW] For std::sort

using namespace std;

// --- Constants ---
constexpr int BOARD_HEIGHT      = 20;
constexpr int BOARD_WIDTH       = 15;
constexpr int BLOCK_SIZE        = 4;
constexpr int NUM_BLOCK_TYPES   = 7;
const string HIGH_SCORE_FILE    = "highscores.txt"; 

// Gameplay settings
constexpr long BASE_DROP_SPEED_US   = 500000; 
constexpr int  DROP_INTERVAL_TICKS  = 5;      

struct Position {
    int x{}, y{};
    Position() = default;
    Position(int _x, int _y) : x(_x), y(_y) {}
};

struct GameState {
    bool running{true};
    bool paused{false};
    
    // Stats
    int score{0};
    int level{1};
    int lines{0};

    // [NEW] Stores the Top 5 High Scores
    vector<int> highScores; 
};

struct Piece {
    int type{0};
    int rotation{0};
    Position pos{5, 0};
};

struct Board {
    char grid[BOARD_HEIGHT][BOARD_WIDTH]{};

    // Initialize with walls and a hole in the ceiling
    void init() {
        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                if (i == BOARD_HEIGHT - 1 || j == 0 || j == BOARD_WIDTH - 1) {
                    grid[i][j] = '#';
                }
                else if (i == 0) {
                    if (j >= 5 && j <= 9) grid[i][j] = ' '; 
                    else grid[i][j] = '#'; 
                }
                else {
                    grid[i][j] = ' ';
                }
            }
        }
    }

    void draw(const GameState& state) const {
        cout << "\033[2J\033[1;1H";
        
        // [UPDATE] Show the #1 Best Score during gameplay
        int topScore = state.highScores.empty() ? 0 : state.highScores[0];

        cout << "SCORE: " << state.score 
             << " | BEST: " << topScore 
             << " | LEVEL: " << state.level << "\n";
             
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
        cout << "        Press R to Restart  \n";
        cout << "        Press Q to Quit     \n";
        cout.flush();
    }

    void animateGameOver(const GameState& state) {
        for (int y = BOARD_HEIGHT - 2; y >= 1; --y) {
            for (int x = 1; x < BOARD_WIDTH - 1; ++x) {
                grid[y][x] = '#'; 
            }
            draw(state);
            usleep(50000); 
        }
    }

    int clearLines() {
        int linesCleared = 0;
        int writeRow = BOARD_HEIGHT - 2;

        for (int readRow = BOARD_HEIGHT - 2; readRow > 0; --readRow) {
            bool full = true;
            for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
                if (grid[readRow][j] == ' ') {
                    full = false;
                    break;
                }
            }

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

    static void setBlockTemplate(int type, char symbol, const int shape[BLOCK_SIZE][BLOCK_SIZE]) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            for (int j = 0; j < BLOCK_SIZE; ++j) {
                templates[type][i][j] = shape[i][j] ? symbol : ' ';
            }
        }
    }

    static void initializeTemplates() {
        static const int TETROMINOES[7][4][4] = {
            { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} }, // I
            { {0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0} }, // O
            { {0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0} }, // T
            { {0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0} }, // S
            { {0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0} }, // Z
            { {0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0} }, // J
            { {0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0} }  // L
        };

        static const char NAMES[7] = {'I','O','T','S','Z','J','L'};
        for (int i = 0; i < 7; i++) setBlockTemplate(i, NAMES[i], TETROMINOES[i]);
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
        
        loadHighScores();
    }

    // --- [NEW] Leaderboard Logic ---

    // Load multiple scores from file
    void loadHighScores() {
        state.highScores.clear();
        ifstream file(HIGH_SCORE_FILE);
        int scoreVal;
        if (file.is_open()) {
            while (file >> scoreVal) {
                state.highScores.push_back(scoreVal);
            }
            file.close();
            
            // Sort just in case the file was messed up
            sort(state.highScores.begin(), state.highScores.end(), greater<int>());
        }
    }

    // Update and Save Top 5
    void updateAndSaveHighScores() {
        // 1. Add current score to the list
        state.highScores.push_back(state.score);

        // 2. Sort descending (Highest first)
        sort(state.highScores.begin(), state.highScores.end(), greater<int>());

        // 3. Keep only top 5
        if (state.highScores.size() > 5) {
            state.highScores.resize(5);
        }

        // 4. Write back to file
        ofstream file(HIGH_SCORE_FILE);
        if (file.is_open()) {
            for (int s : state.highScores) {
                file << s << endl;
            }
            file.close();
        }
    }

    // --- Terminal Handling ---
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
        if (read(STDIN_FILENO, &ch, 1) > 0) return ch;
        return 0;
    }

    void flushInput() const {
        tcflush(STDIN_FILENO, TCIFLUSH);
    }

    // --- Game Logic ---

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

                if (yt >= 0 && !isInsidePlayfield(xt, yt)) return false;
                if (yt >= 0 && board.grid[yt][xt] != ' ') return false;
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
    
    // Feature: Restart Game Logic
    void resetGame() {
        // Save before wiping if game was running
        updateAndSaveHighScores();

        board.init(); 
        state.score = 0;
        state.lines = 0;
        state.level = 1;
        state.running = true;
        state.paused = false;
        dropSpeedUs = BASE_DROP_SPEED_US;
        dropCounter = 0;
        
        spawnNewPiece();
        
        cout << "\033[2J\033[1;1H"; 
        cout << ">>> GAME RESTARTED <<<";
        cout.flush();
        usleep(500000); 
    }

    bool lockPieceAndCheck() {
        placePiece(currentPiece, true);
        
        int lines = board.clearLines();
        if (lines > 0) {
            state.lines += lines;
            state.score += (lines * 100 * state.level);
            
            // Note: We only update the file on Game Over to avoid lag,
            // but we could check against state.highScores[0] here for real-time display.

            if (state.lines / 5 > state.level - 1) {
                 state.level++;
                 dropSpeedUs = max(100000L, dropSpeedUs - 50000); 
            }
        }

        spawnNewPiece();
        if (!state.running) {
            return false;
        }
        return true;
    }

    void softDrop() {
        if (canMove(0, 1, currentPiece.rotation)) currentPiece.pos.y++;
        else { state.running = lockPieceAndCheck(); dropCounter = 0; }
    }

    void hardDrop() {
        while (canMove(0, 1, currentPiece.rotation)) currentPiece.pos.y++;
        state.running = lockPieceAndCheck();
        dropCounter = 0;
    }

    void handleInput() {
        char c = getInput();
        if (c == 0) return;

        if (c == 'r') { resetGame(); return; }
        if (c == 'p') {
            state.paused = !state.paused;
            if (state.paused) board.drawPause();
            return;
        }
        if (state.paused) {
            if (c == 'q') state.running = false;
            return;
        }

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
        }
    }

    void handleGravity() {
        if (!state.running || state.paused) return;
        ++dropCounter;
        if (dropCounter < DROP_INTERVAL_TICKS) return;
        dropCounter = 0;
        if (canMove(0, 1, currentPiece.rotation)) currentPiece.pos.y++;
        else state.running = lockPieceAndCheck();
    }

    void run() {
        BlockTemplate::initializeTemplates();
        board.init();
        
        for (int y = 1; y < BOARD_HEIGHT - 1; ++y) {
            for (int x = 1; x < BOARD_WIDTH - 1; ++x) {
                board.grid[y][x] = ' ';
            }
        }

        enableRawMode();
        spawnNewPiece();

        cout << "Tetris Game - Starting...\n";
        usleep(500000);

        bool appRunning = true;
        while (appRunning) {
            
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
            
            // --- GAME OVER ---
            
            // [NEW] Update Leaderboard
            updateAndSaveHighScores();

            // Run Animation
            board.animateGameOver(state);

            // [NEW] Display Top 5 Leaderboard
            cout << "\n   === GAME OVER ===   \n";
            cout << "   Current Score: " << state.score << "\n\n";
            
            cout << "   --- LEADERBOARD --- \n";
            for (size_t i = 0; i < state.highScores.size(); ++i) {
                // Determine suffix (st, nd, rd, th)
                string suffix = "th";
                if (i == 0) suffix = "st";
                else if (i == 1) suffix = "nd";
                else if (i == 2) suffix = "rd";

                cout << "   " << (i + 1) << suffix << ": " << state.highScores[i];
                if (state.score > 0 && state.score == state.highScores[i]) {
                    cout << " <NEW!"; // Highlight if this is the new score
                }
                cout << "\n";
            }
            cout << "\n   [R] Restart  [Q] Quit\n";
            
            bool waitingForAction = true;
            while (waitingForAction) {
                char c = getInput();
                if (c == 'q') {
                    appRunning = false;
                    waitingForAction = false;
                } else if (c == 'r') {
                    resetGame(); 
                    waitingForAction = false;
                }
                usleep(100000);
            }
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
